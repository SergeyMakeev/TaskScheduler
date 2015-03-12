#pragma once

namespace MT
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class InterlockedInt
	{
		volatile long value;
	public:

		InterlockedInt()
		{
		}

		InterlockedInt(int v)
			: value(v)
		{
		}

		void Add(int sum) 
		{
			InterlockedExchangeAdd(&value, sum);
		}

		int Inc()
		{
			return InterlockedIncrement(&value);
		}

		int Dec()
		{
			return InterlockedDecrement(&value);
		}

		int Get()
		{
			return value;
		}

		void Set(int val)
		{
			value = val;
		}
	};
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}