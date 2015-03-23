#pragma once

#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <limits.h>
#include <stdlib.h>

#include "../Common/Thread.h"

namespace MT
{
	class _Fiber;

	class Thread : public ThreadBase
	{
		pthread_t thread;
		pthread_attr_t threadAttr;

		void * stackBase;
		size_t stackSize;

		bool isStarted;

		static void* ThreadFuncInternal(void *pThread)
		{
			Thread * self = (Thread *)pThread;

			self->func(self->funcData);

			pthread_exit(nullptr);
		}

	public:

		Thread()
			: stackBase(nullptr)
			, stackSize(0)
			, isStarted(false)
		{
		}

		void* GetStackBase()
		{
			return stackBase;
		}

		size_t GetStackSize()
		{
			return stackSize;
		}


		void Start(size_t _stackSize, TThreadEntryPoint entryPoint, void *userData)
		{
			ASSERT(!isStarted, "Thread already stared");

			ASSERT(func == nullptr, "Thread already started");

			func = entryPoint;
			funcData = userData;

			stackSize = _stackSize;

			ASSERT(stackSize >= PTHREAD_STACK_MIN, "Thread stack to small");

			stackBase = (void *)malloc(stackSize);

			int err = pthread_attr_init(&threadAttr);
			ASSERT(err == 0, "pthread_attr_init - error");

			err = pthread_attr_setstack(&threadAttr, stackBase, stackSize);
			ASSERT(err == 0, "pthread_attr_setstack - error");

			err = pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_JOINABLE);
			ASSERT(err == 0, "pthread_attr_setdetachstate - error");

			err = pthread_create(&thread, &threadAttr, ThreadFuncInternal, this);
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

			if (stackBase)
			{
				free(stackBase);
				stackBase = nullptr;
			}
			stackSize = 0;

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
			{
				continue;
			}
		}

	};


}


