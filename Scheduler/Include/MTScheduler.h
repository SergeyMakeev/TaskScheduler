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


		template<class T>
		internal::GroupedTask GetGroupedTask(TaskGroup::Type group, T * src) const
		{
			internal::TaskDesc desc(T::TaskEntryPoint, (void*)(src));
			return internal::GroupedTask(desc, group);
		}

		//template specialization for FiberContext*
		template<>
		internal::GroupedTask GetGroupedTask(TaskGroup::Type group, FiberContext ** src) const
		{
			ASSERT(group == TaskGroup::GROUP_UNDEFINED, "Group must be GROUP_UNDEFINED");
			FiberContext * fiberContext = *src;
			internal::GroupedTask groupedTask(fiberContext->currentTask, fiberContext->currentGroup);
			groupedTask.awaitingFiber = fiberContext;
			return groupedTask;
		}


		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Distributes task to threads:
		// | Task1 | Task2 | Task3 | Task4 | Task5 | Task6 |
		// ThreadCount = 4
		// Thread0: Task1, Task5
		// Thread1: Task2, Task6
		// Thread2: Task3
		// Thread3: Task4
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		template<class TTask>
		bool DistibuteDescriptions(TaskGroup::Type group, TTask* taskArray, fixed_array<internal::GroupedTask>& descriptions, fixed_array<internal::TaskBucket>& buckets) const
		{
			size_t index = 0;

			for (size_t bucketIndex = 0; (bucketIndex < buckets.size()) && (index < descriptions.size()); ++bucketIndex)
			{
				size_t bucketStartIndex = index;

				for (size_t i = bucketIndex; i < descriptions.size(); i += buckets.size())
				{
					descriptions[index++] = GetGroupedTask(group, &taskArray[i]);
				}

				buckets[bucketIndex] = internal::TaskBucket(&descriptions[bucketStartIndex], index - bucketStartIndex);
			}

			ASSERT(index == descriptions.size(), "Sanity check")

			return index > 0;
		}

	public:

		TaskScheduler();
		~TaskScheduler();

		template<class TTask>
		void RunAsync(TaskGroup::Type group, TTask* taskArray, uint32 count);

		bool WaitGroup(TaskGroup::Type group, uint32 milliseconds);
		bool WaitAll(uint32 milliseconds);

		bool IsEmpty();

		uint32 GetWorkerCount() const;

		bool IsWorkerThread() const;
	};

	template<class TTask>
	void TaskScheduler::RunAsync(TaskGroup::Type group, TTask* taskArray, uint32 count)
	{
		ASSERT(!IsWorkerThread(), "Can't use RunAsync inside Task. Use FiberContext.RunAsync() instead.");

		fixed_array<internal::GroupedTask> buffer(ALLOCATE_ON_STACK(internal::GroupedTask, count), count);

		size_t bucketCount = Min(threadsCount, count);
		fixed_array<internal::TaskBucket>	buckets(ALLOCATE_ON_STACK(internal::TaskBucket, bucketCount), bucketCount);

		DistibuteDescriptions(group, taskArray, buffer, buckets);
		RunTasksImpl(buckets, nullptr, false);
	}




}
