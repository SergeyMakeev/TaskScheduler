#pragma once

#include <pthread.h>
#include <unistd.h>
#include <time.h>

namespace MT
{
	class _Fiber;

	class Thread
	{
		void * funcData;
		TThreadEntryPoint func;

		pthread_t thread;
		pthread_attr_t threadAttr;

		bool isStarted;

		static void* ThreadFuncInternal(void *pThread)
		{
			Thread * self = (Thread *)pThread;

			self->func(self->funcData);

			pthread_exit(nullptr);
		}

	private:

		Thread(const Thread &) {}
		void operator=(const Thread &) {}

	public:

		Thread()
			: funcData(nullptr)
			, func(nullptr)
			, isStarted(false)
		{
		}

		~Thread()
		{
		}

		void Start(size_t stackSize, TThreadEntryPoint entryPoint, void *userData)
		{
			ASSERT(!isStarted, "Thread already stared");

			ASSERT(func == nullptr, "Thread already started");

			func = entryPoint;
			funcData = userData;

			int err = pthread_attr_init(&threadAttr);
			ASSERT(err == 0, "pthread_attr_init - error");

			err = pthread_attr_setstacksize(&threadAttr, stackSize);
			ASSERT(err == 0, "pthread_attr_setstacksize - error");

			err = pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_JOINABLE);
			ASSERT(err == 0, "pthread_attr_setdetachstate - error");

			err = pthread_create(&thread, &threadAttr, ThreadFuncInternal, userData);
			ASSERT(err == 0, "pthread_create - error");

			isStarted = true;
		}

		void Stop()
		{
			ASSERT(isStarted, "Thread is not started");

			if (func == nullptr)
			{
				return;
			}

			void *threadStatus = nullptr;
			int err = pthread_join(thread, &threadStatus);
			ASSERT(err == 0, "pthread_join - error");

			err = pthread_attr_destroy(&threadAttr);
			ASSERT(err == 0, "pthread_attr_destroy - error");

			func = nullptr;
			funcData = nullptr;

			isStarted = false;
		}

		bool IsCurrentThread() const
		{
			if(!isStarted)
			{
				return false;
			}

			pthread_t callThread = pthread_self();
			if (pthread_equal(callThread, thread))
			{
					return true;
			}
			return false;
		}

		static int GetNumberOfHardwareThreads()
		{
      			long numberOfProcessors = sysconf( _SC_NPROCESSORS_ONLN );
			return (int)numberOfProcessors;
		}

		static void Sleep(uint32 milliseconds)
		{
      struct timespec req = {0};
      time_t sec = (int)(milliseconds/1000);
      milliseconds = milliseconds - (sec*1000);
      req.tv_sec = sec;
      req.tv_nsec = milliseconds * 1000000L;
      while (nanosleep(&req,&req) == -1 )
           continue;
		}

	};


}


