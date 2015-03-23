#pragma once
#include <sys/time.h>

namespace MT
{
	__inline int64 GetTimeMicroSeconds()
	{
		struct timeval te;
		gettimeofday(&te, nullptr);
		return te.tv_sec*1000000LL + te.tv_usec;
	}

	__inline int64 GetTimeMilliSeconds()
	{
		return GetTimeMicroSeconds() / 1000;
	}
}
