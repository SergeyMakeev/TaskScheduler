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

		AtomicInt() : value(0)
		{
		}

		AtomicInt(int v)
			: value(v)
		{
		}

		int Add(int sum)
		{
			return __sync_add_and_fetch(&value, sum);
		}

		int Inc()
		{
			return __sync_add_and_fetch(&value, 1);
		}

		int Dec()
		{
			return __sync_sub_and_fetch(&value, 1);
		}

		int Get() const
		{
			return value;
		}

		int Set(int val)
		{
			return __sync_lock_test_and_set(&value, val);
		}

		// The function returns the initial value.
		int CompareAndSwap(int compareValue, int newValue)
		{
			return __sync_val_compare_and_swap(&value, compareValue, newValue);
		}

	};

}
