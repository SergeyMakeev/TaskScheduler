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
	template<int N>
	struct is_power_of_two
	{
		enum {value = N && !(N & (N - 1))};
	};


	/// \class ConcurrentRingBuffer
	/// \brief Very naive implementation of thread safe ring buffer. When ring buffer is full and a subsequent write is performed, then it starts overwriting the oldest data. 
	template<typename T, size_t numElements>
	class ConcurrentRingBuffer
	{
		MT::Mutex mutex;

		T buffer[numElements];

		size_t writeIndex;
		size_t readIndex;
		size_t size;

	private:

		ConcurrentRingBuffer(const ConcurrentRingBuffer&) {}
		void operator=(const ConcurrentRingBuffer&) {}


		size_t NextIndex(size_t index)
		{
			size_t ret = index + 1;
			size_t mask = (numElements - 1);
			return (ret & mask);
		}

	public:

		ConcurrentRingBuffer()
			: writeIndex(0)
			, readIndex(0)
			, size(0)
		{
			static_assert(std::is_pod<T>::value == true, "Only POD types allowed to use in MT::ConcurrentRingBuffer");
			static_assert(is_power_of_two<numElements>::value == true, "NumElements used in MT::ConcurrentRingBuffer must be power of two");
		}

		void Push(const T & item)
		{
			MT::ScopedGuard guard(mutex);

			buffer[writeIndex] = item;
			if (size >= numElements)
			{
				// RingBuffer is full. Overwrite old data.
				readIndex = NextIndex(readIndex);
			} else
			{
				size++;
			}

			writeIndex = NextIndex(writeIndex);
		}

		size_t PopAll(T * dstBuffer, size_t dstBufferSize)
		{
			MT::ScopedGuard guard(mutex);

			size_t elementsCount = size;
			elementsCount = Min(elementsCount, dstBufferSize);

			for (size_t i = 0; i < elementsCount; i++)
			{
				dstBuffer[i] = buffer[readIndex];
				readIndex = NextIndex(readIndex);
			}

			size -= elementsCount;
			return elementsCount;
		}




	};

}