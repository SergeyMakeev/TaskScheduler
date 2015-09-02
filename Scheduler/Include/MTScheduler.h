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
#include <MTStackArray.h>
#include <MTArrayView.h>
#include <MTThreadContext.h>
#include <MTFiberContext.h>
#include <MTTaskBase.h>

#ifdef MT_INSTRUMENTED_BUILD
#include <MTMicroWebSrv.h>
#endif

namespace MT
{
	const uint32 MT_MAX_THREAD_COUNT = 64;
	const uint32 MT_MAX_FIBERS_COUNT = 128;
	const uint32 MT_MAX_GROUPS_COUNT = 256;
	const uint32 MT_SCHEDULER_STACK_SIZE = 131072;
	const uint32 MT_FIBER_STACK_SIZE = 32768;

	namespace internal
	{
		struct ThreadContext;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Task scheduler
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class TaskScheduler
	{
		friend class FiberContext;
		friend struct internal::ThreadContext;



		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Task group description
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Application can assign task group to task and later wait until group was finished.
		class TaskGroupDescription
		{
			AtomicInt inProgressTaskCount;
			Event allDoneEvent;

			//Tasks awaiting group through FiberContext::WaitGroupAndYield call
			ConcurrentQueueLIFO<FiberContext*> waitTasksQueue;

		public:

			bool debugIsFree;


		private:

			TaskGroupDescription(TaskGroupDescription& ) {}
			void operator=(const TaskGroupDescription&) {}

		public:

			TaskGroupDescription()
			{
				inProgressTaskCount.Set(0);
				allDoneEvent.Create( EventReset::MANUAL, true );
				debugIsFree = true;
			}

			int GetTaskCount() const { return inProgressTaskCount.Get(); }
			ConcurrentQueueLIFO<FiberContext*> & GetWaitQueue() { return waitTasksQueue; }
			int Dec() { return inProgressTaskCount.Dec(); }
			int Inc() { return inProgressTaskCount.Inc(); }
			int Add(int sum) { return inProgressTaskCount.Add(sum); }
			void Signal() { allDoneEvent.Signal(); }
			void Reset() { allDoneEvent.Reset(); }
			bool Wait(uint32 milliseconds) { return allDoneEvent.Wait(milliseconds); }
		};


		// Thread index for new task
		AtomicInt roundRobinThreadIndex;

		// Started threads count
		AtomicInt startedThreadsCount;

		// Threads created by task manager
		volatile uint32 threadsCount;
		internal::ThreadContext threadContext[MT_MAX_THREAD_COUNT];

		// All groups task statistic
		TaskGroupDescription allGroups;

		// Groups pool
		ConcurrentQueueLIFO<TaskGroup> availableGroups;

		//
		TaskGroupDescription groupStats[MT_MAX_GROUPS_COUNT];

		// Fibers pool
		ConcurrentQueueLIFO<FiberContext*> availableFibers;

		// Fibers context
		FiberContext fiberContext[MT_MAX_FIBERS_COUNT];

#ifdef MT_INSTRUMENTED_BUILD
		int32 webServerPort;
		profile::MicroWebServer profilerWebServer;
		int64 startTime;
#endif

		FiberContext* RequestFiberContext(internal::GroupedTask& task);
		void ReleaseFiberContext(FiberContext* fiberExecutionContext);
		void RunTasksImpl(ArrayView<internal::TaskBucket>& buckets, FiberContext * parentFiber, bool restoredFromAwaitState);
		TaskGroupDescription & GetGroupDesc(TaskGroup group);

		static void ThreadMain( void* userData );
		static void FiberMain( void* userData );
		static bool TryStealTask(internal::ThreadContext& threadContext, internal::GroupedTask & task, uint32 workersCount);

		static FiberContext* ExecuteTask (internal::ThreadContext& threadContext, FiberContext* fiberContext);

	public:

		/// \brief Initializes a new instance of the TaskScheduler class.
		/// \param workerThreadsCount Worker threads count. Automatically determines the required number of threads if workerThreadsCount set to 0
		TaskScheduler(uint32 workerThreadsCount = 0);
		~TaskScheduler();

		template<class TTask>
		void RunAsync(TaskGroup group, TTask* taskArray, uint32 taskCount);

		bool WaitGroup(TaskGroup group, uint32 milliseconds);
		bool WaitAll(uint32 milliseconds);

		TaskGroup CreateGroup();
		void ReleaseGroup(TaskGroup group);

		bool IsEmpty();

		uint32 GetWorkerCount() const;

		bool IsWorkerThread() const;

#ifdef MT_INSTRUMENTED_BUILD

		size_t GetProfilerEvents(uint32 workerIndex, MT::ProfileEventDesc * dstBuffer, size_t dstBufferSize);
		void UpdateProfiler();
		int32 GetWebServerPort() const;

		inline int64 GetStartTime() const
		{
			return startTime;
		}
		
#endif
	};
}

#include "MTScheduler.inl"
#include "MTFiberContext.inl"
