#pragma once

#include <MTTools.h>
#include <MTPlatform.h>
#include <MTConcurrentQueueLIFO.h>
#include <MTStackArray.h>
#include <MTFixedArray.h>
#include <MTThreadContext.h>
#include <MTFiberContext.h>
#include <MTTaskBase.h>

namespace MT
{
	const uint32 MT_MAX_THREAD_COUNT = 32;
	const uint32 MT_MAX_FIBERS_COUNT = 128;
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

		struct GroupStats
		{
			AtomicInt inProgressTaskCount;
			Event allDoneEvent;

			GroupStats()
			{
				inProgressTaskCount.Set(0);
				allDoneEvent.Create( EventReset::MANUAL, true );
			}
		};

		// Thread index for new task
		AtomicInt roundRobinThreadIndex;

		// Threads created by task manager
		uint32 threadsCount;
		internal::ThreadContext threadContext[MT_MAX_THREAD_COUNT];

		// Per group task statistic
		GroupStats groupStats[TaskGroup::COUNT];

		// All groups task statistic
		GroupStats allGroupStats;


		//Task awaiting group through FiberContext::WaitGroupAndYield call
		ConcurrentQueueLIFO<FiberContext*> waitTaskQueues[TaskGroup::COUNT];


		// Fibers pool
		ConcurrentQueueLIFO<FiberContext*> availableFibers;

		// Fibers context
		FiberContext fiberContext[MT_MAX_FIBERS_COUNT];

		FiberContext* RequestFiberContext(internal::GroupedTask& task);
		void ReleaseFiberContext(FiberContext* fiberExecutionContext);

		void RunTasksImpl(fixed_array<internal::TaskBucket>& buckets, FiberContext * parentFiber, bool restoredFromAwaitState);

		static void ThreadMain( void* userData );
		static void FiberMain( void* userData );
		static FiberContext* ExecuteTask (internal::ThreadContext& threadContext, FiberContext* fiberContext);

	public:

		TaskScheduler();
		~TaskScheduler();

		template<class TTask>
		void RunAsync(TaskGroup::Type group, TTask* taskArray, uint32 taskCount);

		bool WaitGroup(TaskGroup::Type group, uint32 milliseconds);
		bool WaitAll(uint32 milliseconds);

		bool IsEmpty();

		uint32 GetWorkerCount() const;

		bool IsWorkerThread() const;
	};
}

#include "MTScheduler.inl"
#include "MTFiberContext.inl"
