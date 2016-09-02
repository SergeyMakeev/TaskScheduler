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
	/// \class TaskQueue
	/// \brief thread safe task queue
	///
	template<typename T>
	class TaskQueue
	{
		static const int32 ALIGNMENT = 16;

	public:
		static const unsigned int CAPACITY = 4096u;
	private:
		static const unsigned int MASK = CAPACITY - 1u;

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

			size_t count = ((end & MASK) - (begin & MASK)) & MASK;
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

		MT_NOCOPYABLE(TaskQueue);

		/// \name Initializes a new instance of the TaskQueue class.
		/// \brief  
		TaskQueue()
			: begin(0)
			, end(0)
		{
			size_t bytesCount = sizeof(T) * CAPACITY;
			data = Memory::Alloc(bytesCount, ALIGNMENT);
		}

		~TaskQueue()
		{
			if (data != nullptr)
			{
				Memory::Free(data);
				data = nullptr;
			}
		}

		/// \brief Add multiple tasks onto queue.
		/// \param itemArray A pointer to first item. Must not be nullptr
		/// \param count Items count.
		bool Add(const T* itemArray, size_t count)
		{
			MT::ScopedGuard guard(mutex);

			if ((Size() + count) >= CAPACITY)
			{
				return false;
			}

			for (size_t i = 0; i < count; ++i)
			{
				size_t index = (end & MASK);
				T* pElement = Buffer() + index;
				CopyCtor( pElement, itemArray[i] );
				end++;
			}

			return true;
		}


		/// \brief Try pop oldest item from queue.
		/// \param item Resultant item
		/// \return true on success or false if the queue is empty.
		bool TryPopOldest(T & item)
		{
			MT::ScopedGuard guard(mutex);

			if (_IsEmpty())
			{
				return false;
			}

			size_t index = (begin & MASK);
			T* pElement = Buffer() + index;
			begin++;
			item = *pElement;
			Dtor(pElement);
			return true;
		}

		/// \brief Try pop a newest item (recently added) from queue.
		/// \param item Resultant item
		/// \return true on success or false if the queue is empty.
		bool TryPopNewest(T & item)
		{
			MT::ScopedGuard guard(mutex);

			if (_IsEmpty())
			{
				return false;
			}

			end--;
			size_t index = (end & MASK);
			T* pElement = Buffer() + index;
			item = *pElement;
			Dtor(pElement);
			return true;
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
