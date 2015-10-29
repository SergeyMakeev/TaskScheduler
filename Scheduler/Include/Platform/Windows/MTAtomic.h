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

namespace MT
{
	//
	// Atomic int
	//
	class AtomicInt
	{
		volatile long value;
	public:

		AtomicInt()
			: value(0)
		{
			MT_ASSERT(IsPointerAligned(&value, 4), "Invalid atomic int alignment");
		}

		explicit AtomicInt(int v)
			: value(v)
		{
			MT_ASSERT(IsPointerAligned(&value, 4), "Invalid atomic int alignment");
		}

		// The function returns the resulting added value.
		int Add(int sum)
		{
			return InterlockedExchangeAdd(&value, sum) + sum;
		}

		// The function returns the resulting incremented value.
		int Inc()
		{
			return InterlockedIncrement(&value);
		}

		// The function returns the resulting decremented value.
		int Dec()
		{
			return InterlockedDecrement(&value);
		}

		int Get() const
		{
			return value;
		}

		// The function returns the initial value.
		int Set(int val)
		{
			return InterlockedExchange(&value, val); 
		}

		// The function returns the initial value.
		int CompareAndSwap(int compareValue, int newValue)
		{
			return InterlockedCompareExchange(&value, newValue, compareValue);
		}
	};


	//
	// Atomic pointer
	//
	template<typename T>
	class AtomicPtr
	{
		volatile T* value;
	
	public:

		AtomicPtr()
			: value(nullptr)
		{
			MT_ASSERT(IsPointerAligned(&value, sizeof(void*)), "Invalid atomic ptr alignment");
		}

		explicit AtomicPtr(T* v)
			: value(v)
		{
			MT_ASSERT(IsPointerAligned(&value, sizeof(void*)), "Invalid atomic ptr alignment");
		}

		T* Get() const
		{
			return (T*)value;
		}

		// The function returns the initial value.
		T* Set(T* val)
		{
			return (T*)InterlockedExchangePointer((PVOID volatile *)&value, (void*)val); 
		}

		// The function returns the initial value.
		T* CompareAndSwap(T* compareValue, T* newValue)
		{
			return (T*)InterlockedCompareExchangePointer((PVOID volatile *)&value, (void*)newValue, (void*)compareValue);
		}
	};


}
