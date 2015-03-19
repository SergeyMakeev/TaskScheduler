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
	};

}
