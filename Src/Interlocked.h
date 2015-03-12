#pragma once

namespace MT
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class InterlockedInt
	{
		volatile long value;
	public:
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