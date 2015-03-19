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
	};

}
