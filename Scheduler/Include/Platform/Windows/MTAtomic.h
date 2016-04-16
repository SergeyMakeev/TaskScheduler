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

#include <MTConfig.h>
#include <intrin.h>
#include <cstdint>
#include <type_traits>
#include <xmmintrin.h>

namespace MT
{
	//
	// Full memory barrier
	//
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	inline void HardwareFullMemoryBarrier()
	{
		_mm_mfence();
	}

	//
	// Signals to the processor to give resources to threads that are waiting for them.
	//
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	inline void YieldCpu()
	{
		_mm_pause();
	}


	//
	// Atomic int (pod type)
	//
	// You must use this type when you need to declare static variable instead of AtomicInt32
	//
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct AtomicInt32Base
	{
		volatile int32 _value;

		// The function returns the resulting added value.
		int32 AddFetch(int32 sum)
		{
			return _InterlockedExchangeAdd((volatile long*)&_value, sum) + sum;
		}

		// The function returns the resulting incremented value.
		int32 IncFetch()
		{
			return _InterlockedIncrement((volatile long*)&_value);
		}

		// The function returns the resulting decremented value.
		int32 DecFetch()
		{
			return _InterlockedDecrement((volatile long*)&_value);
		}

		int32 Load() const
		{
			return _value;
		}

		// The function returns the initial value.
		int32 Store(int32 val)
		{
			return _InterlockedExchange((volatile long*)&_value, val); 
		}

		// The function returns the initial value.
		int32 CompareAndSwap(int32 compareValue, int32 newValue)
		{
			return _InterlockedCompareExchange((volatile long*)&_value, newValue, compareValue);
		}

		// Relaxed operation: there are no synchronization or ordering constraints
		int32 LoadRelaxed() const
		{
			return _value;
		}

		// Relaxed operation: there are no synchronization or ordering constraints
		void StoreRelaxed(int32 val)
		{
			_value = val;
		}

	};


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	static_assert(sizeof(int32) == sizeof(long), "Incompatible types, Interlocked* will fail.");



	//
	// Atomic pointer (pod type)
	//
	// You must use this type when you need to declare static variable instead of AtomicInt32
	//
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct AtomicPtrBase
	{
		volatile const void* _value;
	
		void* Load() const
		{
			return (void*)_value;
		}

		// The function returns the initial value.
		const void* Store(const void* val)
		{
#ifndef MT_PTR64
			static_assert(sizeof(long) == sizeof(void*), "Incompatible types, _InterlockedExchange will fail");
			return (void*)_InterlockedExchange((volatile long*)&_value, (long)val); 
#else
			return _InterlockedExchangePointer((void* volatile*)&_value, (void*)val); 
#endif
		}

		// The function returns the initial value.
		const void* CompareAndSwap(const void* compareValue, const void* newValue)
		{
#ifndef MT_PTR64
			static_assert(sizeof(long) == sizeof(void*), "Incompatible types, _InterlockedCompareExchange will fail");
			return (void*)_InterlockedCompareExchange((volatile long*)&_value, (long)newValue, (long)compareValue);
#else
			return _InterlockedCompareExchangePointer((void* volatile*)&_value, (void*)newValue, (void*)compareValue);
#endif
		}

		// Relaxed operation: there are no synchronization or ordering constraints
		const void* LoadRelaxed() const
		{
			return (const void*)_value;
		}

		// Relaxed operation: there are no synchronization or ordering constraints
		void StoreRelaxed(const void* val)
		{
			_value = val;
		}

	};



}


#endif
