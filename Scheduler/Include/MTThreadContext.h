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

#include <MTTools.h>
#include <MTPlatform.h>
#include <MTConcurrentQueueLIFO.h>
#include <MTConcurrentRingBuffer.h>
#include <MTGroupedTask.h>

namespace MT
{
	class FiberContext;
	class TaskScheduler;



	namespace internal
	{
		static const size_t TASK_BUFFER_CAPACITY = 4096;


		namespace ThreadState
		{
			const uint32 ALIVE = 0;
			const uint32 EXIT = 1;
		};

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Thread (Scheduler fiber) context
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		struct ThreadContext
		{
			FiberContext* lastActiveFiberContext;

			// pointer to task manager
			TaskScheduler* taskScheduler;

			// thread
			Thread thread;

			// scheduler fiber
			Fiber schedulerFiber;

			// task queue awaiting execution
			ConcurrentQueueLIFO<internal::GroupedTask> queue;

			// new task was arrived to queue event
			Event hasNewTasksEvent;

			// whether thread is alive
			AtomicInt32 state;

			// Temporary buffer, fixed size = TASK_BUFFER_CAPACITY
			void* descBuffer;

			// Thread index
			uint32 workerIndex;

			// Thread random number generator
			LcgRandom random;

			// prevent false sharing between threads
			uint8 cacheline[64];

			ThreadContext();
			~ThreadContext();

			void RestoreAwaitingTasks(TaskGroup taskGroup);

			void SetThreadIndex(uint32 threadIndex);

#ifdef MT_INSTRUMENTED_BUILD
			
			void NotifyThreadCreate(uint32 threadIndex);
			void NotifyThreadStart(uint32 threadIndex);
			void NotifyThreadStop(uint32 threadIndex);

			void NotifyTaskFinished(const internal::TaskDesc & desc);
			void NotifyTaskResumed(const internal::TaskDesc & desc);
			void NotifyTaskYielded(const internal::TaskDesc & desc);

			void NotifyThreadIdleBegin(uint32 threadIndex);
			void NotifyThreadIdleEnd(uint32 threadIndex);

#endif
		};

	}

}
