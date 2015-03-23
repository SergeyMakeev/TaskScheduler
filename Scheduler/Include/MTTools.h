#pragma once
#include <MTTypes.h>
#include <MTPlatform.h>

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
		uint64 startMicroSeconds;
	public:
		Timer() : startMicroSeconds(MT::GetTimeMicroSeconds())
		{
		}

		uint32 GetPastMicroSeconds() const
		{
			return (uint32)(MT::GetTimeMicroSeconds() - startMicroSeconds);
		}

		uint32 GetPastMilliSeconds() const
		{
			return (uint32)((MT::GetTimeMicroSeconds() - startMicroSeconds) / 1000);
		}
	};
}

