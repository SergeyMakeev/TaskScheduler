#pragma once

namespace MT
{
	class ThreadBase
	{
	protected:
		void * funcData;
		TThreadEntryPoint func;
	public:

		MT_NOCOPYABLE(ThreadBase);

		ThreadBase()
			: funcData(nullptr)
			, func(nullptr)
		{
		}

		static void SpinSleep(uint32 milliseconds)
		{
			int64 desiredTime = GetTimeMilliSeconds() + milliseconds;
			while(GetTimeMilliSeconds() <= desiredTime) {}
		}
	};
}


