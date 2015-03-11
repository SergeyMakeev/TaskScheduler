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

		void Inc()
		{
			InterlockedIncrement(&value);
		}

		void Dec()
		{
			InterlockedDecrement(&value);
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