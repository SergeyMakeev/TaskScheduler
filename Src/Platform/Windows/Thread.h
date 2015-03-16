#pragma once


namespace MT
{

	class Thread
	{
		void * funcData;
		TThreadEntryPoint func;

		DWORD threadId;
		HANDLE thread;

		static DWORD __stdcall ThreadFuncInternal(void *pThread)
		{
			Thread * self = (Thread *)pThread;

			self->func(self->funcData);

			ExitThread(0);
		}

	private:

		Thread(const Thread &) {}
		void operator=(const Thread &) {}

	public:

		Thread()
			: func(nullptr)
			, funcData(nullptr)
			, thread(nullptr)
			, threadId(0)
		{
		}

		~Thread()
		{
			ASSERT(thread == nullptr, "Thread is not stopped!")
		}

		void Start(size_t stackSize, TThreadEntryPoint entryPoint, void *userData)
		{
			func = entryPoint;
			funcData = userData;
			thread = ::CreateThread( nullptr, stackSize, ThreadFuncInternal, this, 0, &threadId );
		}

		void Stop()
		{
			if (thread == nullptr)
			{
				return;
			}

			::WaitForSingleObject(thread, INFINITE);
			CloseHandle(thread);
			thread = nullptr;
		}

		bool IsCurrentThread() const
		{
			return threadId == ::GetCurrentThreadId();
		}

		static int GetNumberOfHardwareThreads()
		{
			SYSTEM_INFO sysinfo;
			::GetSystemInfo( &sysinfo );
			return sysinfo.dwNumberOfProcessors;
		}

	};


}


