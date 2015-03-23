#pragma once

namespace MT
{
	class ThreadBase
	{
	protected:
		void * funcData;
		TThreadEntryPoint func;
	private:
		ThreadBase(const ThreadBase &) {}
		void operator=(const ThreadBase &) {}
	public:

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


