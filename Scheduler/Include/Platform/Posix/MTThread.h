// The MIT License (MIT)
//
// 	Copyright (c) 2015 Sergey Makeev, Vadim Slyusarev
//
// 	Permission is hereby granted, free of charge, to any person obtaining a copy
// 	of this software and associated documentation files (the "Software"), to deal
// 	in the Software without restriction, including without limitation the rights
// 	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// 	copies of the Software, and to permit persons to whom the Software is
// 	furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
// 	all copies or substantial portions of the Software.
//
// 	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// 	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// 	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// 	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// 	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// 	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// 	THE SOFTWARE.

#pragma once

#ifndef __MT_THREAD__
#define __MT_THREAD__

#include <MTConfig.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <limits.h>
#include <stdlib.h>

#if MT_PLATFORM_OSX
#include <thread>
#endif

#define _DARWIN_C_SOURCE
#include <sys/mman.h>

#ifndef MAP_ANONYMOUS
    #define MAP_ANONYMOUS MAP_ANON
#endif

#ifndef MAP_STACK
    #define MAP_STACK (0)
#endif

#include <Platform/Common/MTThread.h>
#include <MTAppInterop.h>

namespace MT
{
	class _Fiber;

	class Thread : public ThreadBase
	{
		pthread_t thread;
		pthread_attr_t threadAttr;

		Memory::StackDesc stackDesc;

    size_t stackSize;

		bool isStarted;

		static void* ThreadFuncInternal(void* pThread)
		{
			Thread* self = (Thread *)pThread;
			self->func(self->funcData);
			return nullptr;
		}

	public:

		Thread()
			: stackSize(0)
			, isStarted(false)
		{
		}

		void* GetStackBottom()
		{
			return stackDesc.stackBottom;
		}

		size_t GetStackSize()
		{
			return stackSize;
		}


		void Start(size_t _stackSize, TThreadEntryPoint entryPoint, void* userData)
		{
			MT_ASSERT(!isStarted, "Thread already stared");

			MT_ASSERT(func == nullptr, "Thread already started");

			func = entryPoint;
			funcData = userData;

			stackDesc = Memory::AllocStack(_stackSize);
			stackSize = stackDesc.GetStackSize();

			MT_ASSERT(stackSize >= PTHREAD_STACK_MIN, "Thread stack to small");

			int err = pthread_attr_init(&threadAttr);
			MT_USED_IN_ASSERT(err);
			MT_ASSERT(err == 0, "pthread_attr_init - error");

			err = pthread_attr_setstack(&threadAttr, stackDesc.stackBottom, stackSize);
			MT_USED_IN_ASSERT(err);
			MT_ASSERT(err == 0, "pthread_attr_setstack - error");

			err = pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_JOINABLE);
			MT_USED_IN_ASSERT(err);
			MT_ASSERT(err == 0, "pthread_attr_setdetachstate - error");

			isStarted = true;

			err = pthread_create(&thread, &threadAttr, ThreadFuncInternal, this);
			MT_USED_IN_ASSERT(err);
			MT_ASSERT(err == 0, "pthread_create - error");
		}

		void Stop()
		{
			MT_ASSERT(isStarted, "Thread is not started");

			if (func == nullptr)
			{
				return;
			}

			void *threadStatus = nullptr;
			int err = pthread_join(thread, &threadStatus);
			MT_USED_IN_ASSERT(err);
			MT_ASSERT(err == 0, "pthread_join - error");

			err = pthread_attr_destroy(&threadAttr);
			MT_USED_IN_ASSERT(err);
			MT_ASSERT(err == 0, "pthread_attr_destroy - error");

			func = nullptr;
			funcData = nullptr;

			if (stackDesc.stackMemory != nullptr)
			{
				Memory::FreeStack(stackDesc);
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
#if MT_PLATFORM_OSX
            return std::thread::hardware_concurrency();
#else
			long numberOfProcessors = sysconf( _SC_NPROCESSORS_ONLN );
			return (int)numberOfProcessors;
#endif
		}

		static void Sleep(uint32 milliseconds)
		{
			struct timespec req;
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


#endif
