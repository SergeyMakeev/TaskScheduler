#pragma once

#include <MTTools.h>
#include <MTPlatform.h>
#include <MTConcurrentQueueLIFO.h>
#include <MTTaskBucket.h>


namespace MT
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Fiber task status
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Task can be completed for several reasons.
	// For example task was done or someone call Yield from the Task body.
	namespace FiberTaskStatus
	{
		enum Type
		{
			UNKNOWN = 0,
			RUNNED = 1,
			FINISHED = 2,
			AWAITING_GROUP = 3,
			AWAITING_CHILD = 4,
		};
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Fiber context
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Context passed to fiber main function
	class FiberContext
	{
	private:

		void RunSubtasksAndYieldImpl(fixed_array<internal::TaskBucket>& buckets);

	public:

		FiberContext();

		template<class TTask>
		void RunSubtasksAndYield(TaskGroup::Type taskGroup, const TTask* taskArray, size_t count);

		template<class TTask>
		void RunAsync(TaskGroup::Type taskGroup, TTask* taskArray, uint32 count);

		void WaitGroupAndYield(TaskGroup::Type group);

		void Reset();

		void SetThreadContext(internal::ThreadContext * _threadContext);
		internal::ThreadContext* GetThreadContext();

		void SetStatus(FiberTaskStatus::Type _taskStatus);
		FiberTaskStatus::Type GetStatus() const;

	private:

		// Active thread context (null if fiber context is not executing now)
		internal::ThreadContext * threadContext;

		// Active task status
		FiberTaskStatus::Type taskStatus;

	public:

		// Active task attached to this fiber
		internal::TaskDesc currentTask;

		// Active task group
		TaskGroup::Type currentGroup;

		// Number of children fibers
		AtomicInt childrenFibersCount;

		// Parent fiber
		FiberContext* parentFiber;

		// System fiber
		Fiber fiber;

		// Prevent false sharing between threads
		uint8 cacheline[64];
	};



	template<class TTask>
	void FiberContext::RunSubtasksAndYield(TaskGroup::Type taskGroup, const TTask* taskArray, size_t count)
	{
		ASSERT(threadContext, "ThreadContext is nullptr");
		ASSERT(count < threadContext->descBuffer.size(), "Buffer overrun!")

			size_t threadsCount = threadContext->taskScheduler->GetWorkerCount();

		fixed_array<internal::GroupedTask> buffer(&threadContext->descBuffer.front(), count);

		size_t bucketCount = Min(threadsCount, count);
		fixed_array<internal::TaskBucket>	buckets(ALLOCATE_ON_STACK(internal::TaskBucket, bucketCount), bucketCount);

		threadContext->taskScheduler->DistibuteDescriptions(taskGroup, taskArray, buffer, buckets);
		RunSubtasksAndYieldImpl(buckets);
	}

	template<class TTask>
	void FiberContext::RunAsync(TaskGroup::Type taskGroup, TTask* taskArray, uint32 count)
	{
		ASSERT(threadContext, "ThreadContext is nullptr");
		ASSERT(threadContext->taskScheduler->IsWorkerThread(), "Can't use RunAsync outside Task. Use TaskScheduler.RunAsync() instead.");

		TaskScheduler& scheduler = *(threadContext->taskScheduler);

		fixed_array<internal::GroupedTask> buffer(&threadContext->descBuffer.front(), count);

		size_t bucketCount = Min(scheduler.GetWorkerCount(), count);
		fixed_array<internal::TaskBucket>	buckets(ALLOCATE_ON_STACK(internal::TaskBucket, bucketCount), bucketCount);

		scheduler.DistibuteDescriptions(taskGroup, taskArray, buffer, buckets);
		scheduler.RunTasksImpl(buckets, nullptr, false);
	}




}
