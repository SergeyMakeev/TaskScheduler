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

#ifndef __MT_ATOMIC__
#define __MT_ATOMIC__

#include <intrin.h>
#include <cstdint>

namespace MT
{
	inline void HardwareFullMemoryBarrier()
	{
		_mm_mfence();
	}

	//
	// Atomic int
	//
	class AtomicInt32
	{
		volatile long value;
	public:

		AtomicInt32()
			: value(0)
		{
			static_assert(sizeof(AtomicInt32) == 4, "Invalid type size");
			MT_ASSERT(IsPointerAligned(&value, 4), "Invalid atomic int alignment");
		}

		explicit AtomicInt32(int v)
			: value(v)
		{
			static_assert(sizeof(AtomicInt32) == 4, "Invalid type size");
			MT_ASSERT(IsPointerAligned(&value, 4), "Invalid atomic int alignment");
		}

		// The function returns the resulting added value.
		int AddFetch(int sum)
		{
			return _InterlockedExchangeAdd(&value, sum) + sum;
		}

		// The function returns the resulting incremented value.
		int IncFetch()
		{
			return _InterlockedIncrement(&value);
		}

		// The function returns the resulting decremented value.
		int DecFetch()
		{
			return _InterlockedDecrement(&value);
		}

		int Load() const
		{
			return value;
		}

		// The function returns the initial value.
		int Store(int val)
		{
			return _InterlockedExchange(&value, val); 
		}

		// The function returns the initial value.
		int CompareAndSwap(int compareValue, int newValue)
		{
			return _InterlockedCompareExchange(&value, newValue, compareValue);
		}

		// Relaxed operation: there are no synchronization or ordering constraints
		int LoadRelaxed() const
		{
			return value;
		}

		// Relaxed operation: there are no synchronization or ordering constraints
		void StoreRelaxed(int val)
		{
			value = val;
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

		T* Load() const
		{
			return (T*)value;
		}

		// The function returns the initial value.
		T* Store(T* val)
		{
#if (UINTPTR_MAX == UINT32_MAX)
			return (T*)_InterlockedExchange((volatile long *)&value, (long)val); 
#else
			return (T*)_InterlockedExchangePointer((PVOID volatile *)&value, (PVOID)val); 
#endif
		}

		// The function returns the initial value.
		T* CompareAndSwap(T* compareValue, T* newValue)
		{
#if (UINTPTR_MAX == UINT32_MAX)
			return (T*)_InterlockedCompareExchange((volatile long *)&value, (long)newValue, (long)compareValue);
#else
			return (T*)_InterlockedCompareExchangePointer((PVOID volatile *)&value, (PVOID)newValue, (PVOID)compareValue);
#endif
		}

		// Relaxed operation: there are no synchronization or ordering constraints
		T* LoadRelaxed() const
		{
			return value;
		}

		// Relaxed operation: there are no synchronization or ordering constraints
		void StoreRelaxed(T* val)
		{
			value = val;
		}

	};


}


#endif