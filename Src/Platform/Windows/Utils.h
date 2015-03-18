#pragma once

namespace MT
{
	inline uint64 GetTimeMS()
	{
		return clock() / (CLOCKS_PER_SEC / 1000);
	}
}

