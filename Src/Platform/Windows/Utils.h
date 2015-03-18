#pragma once

namespace MT
{
	uint64 GetTimeMS()
	{
		return clock() / (CLOCK_TIME_PER_SEC / 1000);
	}
}

