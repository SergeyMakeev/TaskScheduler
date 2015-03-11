#pragma once
#include "Platform.h"

namespace MT
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class InterlockedInt
	{
		volatile long value;
	public:
		INLINE long Add(int sum) 
		{
			return InterlockedAdd(&value, sum);
		}

		INLINE void Inc()
		{
			InterlockedIncrement(&value);
		}

		INLINE void Dec()
		{
			InterlockedDecrement(&value);
		}

		INLINE void Get()
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