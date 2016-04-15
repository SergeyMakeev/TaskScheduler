// The MIT License (MIT)
// 
// 	Copyright (c) 2015 Sergey Makeev, Vadim Slyusarev
// 
// 	Permission is hereby granted, free of charge, to any person obtaining a copy
// 	of this software and associated documentation files (the "Software"), to deal
// 	in the Software without restriction, including without limitation the rights
// 	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// 	copies of the Software, and to permit persons to whom the Software is
// 	furnished to do so, subject to the following conditions:
// 
//  The above copyright notice and this permission notice shall be included in
// 	all copies or substantial portions of the Software.
// 
// 	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// 	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// 	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// 	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// 	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// 	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// 	THE SOFTWARE.

#pragma once

#include <vector>

#include <MTPlatform.h>
#include <MTTools.h>
#include <MTAppInterop.h>


namespace MT
{
	/// \class ConcurrentQueueLIFO
	/// \brief Naive locking implementation of thread safe last in, first out (LIFO) queue
	///
	/// for lock-free implementation read Molecule Engine blog by Stefan Reinalter
	/// http://blog.molecular-matters.com/2015/09/25/job-system-2-0-lock-free-work-stealing-part-3-going-lock-free/
	///
	template<typename T>
	class ConcurrentQueueLIFO
	{
		static const unsigned int MAX_JOBS_COUNT = 4096u;
		static const unsigned int MASK = MAX_JOBS_COUNT - 1u;

		MT::Mutex mutex;
		
		void* data;

		size_t begin;
		size_t end;
		
		inline T* Buffer()
		{
			return (T*)(data);
		}

		inline void CopyCtor(T* element, const T & val)
		{
			new(element) T(val);
		}

		inline void MoveCtor(T* element, T && val)
		{
			new(element) T(std::move(val));
		}

		inline void Dtor(T* element)
		{
			MT_UNUSED(element);
			element->~T();
		}

		inline bool _IsEmpty() const
		{
			return (begin == end);
		}

		inline size_t Size() const
		{
			if (_IsEmpty())
			{
				return 0;
			}

			size_t count = (end & MASK) - (begin & MASK);
			return count;
		}

		inline void Clear()
		{
			size_t queueSize = Size();
			for (size_t i = 0; i < queueSize; i++)
			{
				T* pElement = Buffer() + ((begin + i) & MASK);
				Dtor(pElement);
			}

			begin = 0;
			end = 0;
		}


	public:

		MT_NOCOPYABLE(ConcurrentQueueLIFO);

		/// \name Initializes a new instance of the ConcurrentQueueLIFO class.
		/// \brief  
		ConcurrentQueueLIFO()
			: begin(0)
			, end(0)
		{
			size_t bytesCount = sizeof(T) * MAX_JOBS_COUNT;
			data = Memory::Alloc(bytesCount);
		}

		~ConcurrentQueueLIFO()
		{
			if (data != nullptr)
			{
				Memory::Free(data);
				data = nullptr;
			}
		}

		/// \brief Push an item onto the top of queue.
		/// \param item item to push
		void Push(const T & item)
		{
			MT::ScopedGuard guard(mutex);

			if ((Size() + 1) >= MAX_JOBS_COUNT)
			{
				MT_REPORT_ASSERT("Queue overflow");
				return;
			}


			T* pElement = Buffer() + (end & MASK);
			CopyCtor(pElement, item );
			end++;
		}

		/// \brief Push an multiple items onto the top of queue.
		/// \param itemArray A pointer to first item. Must not be nullptr
		/// \param count Items count.
		void PushRange(const T* itemArray, size_t count)
		{
			MT::ScopedGuard guard(mutex);

			if ((Size() + count) >= MAX_JOBS_COUNT)
			{
				MT_REPORT_ASSERT("Queue overflow");
				return;
			}

			for (size_t i = 0; i < count; ++i)
			{
				T* pElement = Buffer() + (end & MASK);
				CopyCtor( pElement, itemArray[i] );
				end++;
			}
		}

		/// \brief Try pop item from the bottom of queue.
		/// \param item Resultant item
		/// \return true on success or false if the queue is empty.
		bool TryPopFront(T & item)
		{
			MT::ScopedGuard guard(mutex);

			if (_IsEmpty())
			{
				return false;
			}

			T* pElement = Buffer() + (begin & MASK);
			begin++;
			item = *pElement;
			Dtor(pElement);
			return true;
		}

		/// \brief Try pop item from the top of queue.
		/// \param item Resultant item
		/// \return true on success or false if the queue is empty.
		bool TryPopBack(T & item)
		{
			MT::ScopedGuard guard(mutex);

			if (_IsEmpty())
			{
				return false;
			}

			end--;
			T* pElement = Buffer() + (end & MASK);
			item = *pElement;
			Dtor(pElement);
			return true;
		}

		/// \brief Pop all items from the queue.
		/// \param dstBuffer A pointer to the buffer to receive queue items.
		/// \param dstBufferSize This variable specifies the size of the dstBuffer buffer.
		/// \return The return value is the number of items written to the buffer.
		size_t PopAll(T * dstBuffer, size_t dstBufferSize)
		{
			MT::ScopedGuard guard(mutex);

			size_t elementsCount = MT::Min(Size(), dstBufferSize);
			for (size_t i = 0; i < elementsCount; i++)
			{
				T* pElement = Buffer() + ((begin + i) & MASK);
				dstBuffer[i] = std::move(*pElement);
			}

			Clear();
			return elementsCount;
		}

		/// \brief Check if queue is empty.
		/// \return true - if queue is empty.
		bool IsEmpty()
		{
			MT::ScopedGuard guard(mutex);

			return _IsEmpty();
		}

	};
}
