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

		static void SpinSleepMicroSeconds(uint32 microseconds)
		{
			int64 desiredTime = GetTimeMicroSeconds() + microseconds;
			while(GetTimeMicroSeconds() <= desiredTime) {}
		}

		static void SpinSleepMilliSeconds(uint32 milliseconds)
		{
			int64 desiredTime = GetTimeMilliSeconds() + milliseconds;
			while(GetTimeMilliSeconds() <= desiredTime) {}
		}

		// obsolete 
		static void SpinSleep(uint32 milliseconds)
		{
			SpinSleepMilliSeconds(milliseconds);
		}

	};
}


