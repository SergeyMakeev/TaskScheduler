#pragma once

#include <unistd.h>

namespace MT
{
	class _Fiber;

	class Thread
	{
		void * funcData;
		TThreadEntryPoint func;

		pthread_t thread;
		pthread_attr_t threadAttr;

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
			: func(nullptr)
			, funcData(nullptr)
		{
		}

		~Thread()
		{
		}

		void Start(size_t stackSize, TThreadEntryPoint entryPoint, void *userData)
		{
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
		}

		void Stop()
		{
			if (func == nullptr)
			{
				return;
			}

			void *threadStatus = nullptr;
			pthread_join(thread, &threadStatus);
			pthread_attr_destroy(&threadAttr);
			func = nullptr;
		}

		bool IsCurrentThread() const
		{
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

	};


}


