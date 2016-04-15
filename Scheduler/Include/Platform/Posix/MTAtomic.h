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

#include <type_traits>

#if MT_SSE_INTRINSICS_SUPPORTED
#include <xmmintrin.h>
#else
#include <unistd.h>
#endif


namespace MT
{

	//
	// Full memory barrier
	//
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	inline void HardwareFullMemoryBarrier()
	{
		__sync_synchronize();
	}

	//
	// Signals to the processor to give resources to threads that are waiting for them.
	//
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	inline void YieldCpu()
	{
#if MT_SSE_INTRINSICS_SUPPORTED
		_mm_pause();
#else
		usleep(0);
#endif
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

		int32 AddFetch(int32 sum)
		{
			return __sync_add_and_fetch(&_value, sum);
		}

		int32 IncFetch()
		{
			return __sync_add_and_fetch(&_value, 1);
		}

		int32 DecFetch()
		{
			return __sync_sub_and_fetch(&_value, 1);
		}

		int32 Load() const
		{
			return _value;
		}

		int32 Store(int32 val)
		{
			return __sync_lock_test_and_set(&_value, val);
		}

		// The function returns the initial value.
		int32 CompareAndSwap(int32 compareValue, int32 newValue)
		{
			return __sync_val_compare_and_swap(&_value, compareValue, newValue);
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


	//
	// Atomic pointer (pod type)
	//
	// You must use this type when you need to declare static variable instead of AtomicInt32
	//
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct AtomicPtrBase
	{
		volatile const void* _value;

		const void* Load() const
		{
			return const_cast<void*>(_value);
		}

		// The function returns the initial value.
		const void* Store(const void* val)
		{
			const void* r = __sync_lock_test_and_set((void**)&_value, (void*)val);
			return r;
		}

		// The function returns the initial value.
		const void* CompareAndSwap(const void* compareValue, const void* newValue)
		{
			const void* r = __sync_val_compare_and_swap((void**)&_value, (void*)compareValue, (void*)newValue);
			return r;
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
