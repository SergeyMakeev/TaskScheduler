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
			return __sync_fetch_and_add(&value, sum);
		}

		int Inc()
		{
			return __sync_fetch_and_add(&value, 1);
		}

		int Dec()
		{
			return __sync_fetch_and_sub(&value, 1);
		}

		int Get()
		{
			return value;
		}

		int Set(int val)
		{
			return __sync_lock_test_and_set(&value, val);
		}
	};

}
