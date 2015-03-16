#pragma once


namespace MT
{
	class _Fiber;

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
			ASSERT(thread == nullptr, "Thread already started");

			func = entryPoint;
			funcData = userData;
			thread = ::CreateThread( nullptr, stackSize, ThreadFuncInternal, this, 0, &threadId );
			ASSERT(thread != nullptr, "Can't create thread");
		}

		void Stop()
		{
			if (thread == nullptr)
			{
				return;
			}

			::WaitForSingleObject(thread, INFINITE);
			BOOL res = CloseHandle(thread);
			ASSERT(res != 0, "Can't close thread handle")
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


