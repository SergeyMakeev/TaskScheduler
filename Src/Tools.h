#pragma once
#include "types.h"
#include "Platform.h"

#define UNUSED(T)

template<class T>
T Min(T a, T b)
{
    return a < b ? a : b;
}

template<class T>
T Max(T a, T b)
{
    return a < b ? b : a;
}

template<class T>
T Clamp(T val, T min, T max)
{
    return Min(max, Max(min, val));
}

namespace MT
{
	class Timer
	{
		uint64 start;
	public:
		Timer() : start(MT::GetTimeMS())
		{
		}

		uint32 Get() const
		{
			return (uint32)(MT::GetTimeMS() - start);
		}
	};
}

