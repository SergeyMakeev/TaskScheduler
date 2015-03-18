#pragma once
#include <sys/time.h>

namespace MT
{
	__inline uint64 GetTimeMS()
	{
		struct timeval te;
    gettimeofday(&te, NULL);
    return te.tv_sec*1000LL + te.tv_usec/1000;
	}
}
