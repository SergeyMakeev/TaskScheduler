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


namespace MT
{
	/// \class ConcurrentQueueLIFO
	/// \brief Very naive implementation of thread safe  last in, first out (LIFO) queue
	template<typename T>
	class ConcurrentQueueLIFO
	{
		MT::Mutex mutex;
		std::vector<T> queue;

	public:

		/// \name Initializes a new instance of the ConcurrentQueueLIFO class.
		/// \brief  
		ConcurrentQueueLIFO()
		{
			queue.reserve(256);
		}

		/// 
		~ConcurrentQueueLIFO() {}

		/// \brief Push an item onto the top of queue.
		/// \param item item to push
		void Push(const T & item)
		{
			MT::ScopedGuard guard(mutex);

			queue.push_back(item);
		}

		/// \brief Push an multiple items onto the top of queue.
		/// \param itemArray A pointer to first item. Must not be nullptr
		/// \param count Items count.
		void PushRange(const T* itemArray, size_t count)
		{
			MT::ScopedGuard guard(mutex);

			for (size_t i = 0; i < count; ++i)
				queue.push_back(itemArray[i]);
		}

		/// \brief Try pop item from the top of queue.
		/// \param item Resultant item
		/// \return true on success or false if the queue is empty.
		bool TryPop(T & item)
		{
			MT::ScopedGuard guard(mutex);

			if (queue.empty())
			{
				return false;
			}
			item = queue.back();
			queue.pop_back();
			return true;
		}

		/// \brief Pop all items from the queue.
		/// \param dstBuffer A pointer to the buffer to receive queue items.
		/// \param dstBufferSize This variable specifies the size of the dstBuffer buffer.
		/// \return The return value is the number of items written to the buffer.
		size_t PopAll(T * dstBuffer, size_t dstBufferSize)
		{
			MT::ScopedGuard guard(mutex);
			size_t elementsCount = Min(queue.size(), dstBufferSize);
			for (size_t i = 0; i < elementsCount; i++)
			{
				dstBuffer[i] = std::move(queue[i]);
			}
			queue.clear();
			return elementsCount;
		}

		/// \brief Check if queue is empty.
		/// \return true - if queue is empty.
		bool IsEmpty()
		{
			MT::ScopedGuard guard(mutex);
			return queue.empty();
		}

	};
}