#pragma once

#include "Tools.h"
#include "Platform.h"
#include "ConcurrentQueueLIFO.h"
#include "Containers.h"

#ifdef Yield
#undef Yield
#endif

namespace MT
{

	const uint32 MT_MAX_THREAD_COUNT = 32;
	const uint32 MT_MAX_FIBERS_COUNT = 128;
	const uint32 MT_SCHEDULER_STACK_SIZE = 131072;
	const uint32 MT_FIBER_STACK_SIZE = 16384;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Task group
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Application can wait until whole group was finished.
	namespace TaskGroup
	{
		enum Type
		{
			GROUP_0 = 0,
			GROUP_1 = 1,
			GROUP_2 = 2,

			COUNT,

			GROUP_UNDEFINED
		};
	}


	class FiberContext;

	typedef void (*TTaskEntryPoint)(FiberContext & context, void* userData);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Task description
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct TaskDesc
	{
		//Task entry point
		TTaskEntryPoint taskFunc;

		//Task user data (task context)
		void* userData;

		TaskDesc()
			: taskFunc(nullptr)
			, userData(nullptr)
		{
		}

		TaskDesc(TTaskEntryPoint _taskFunc, void* _userData)
			: taskFunc(_taskFunc)
			, userData(_userData)
		{
		}

		bool IsValid()
		{
			return (taskFunc != nullptr);
		}
	};

	struct GroupedTask;
	struct ThreadContext;

	class TaskScheduler;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct TaskBucket
	{
		GroupedTask* tasks;
		size_t count;
		TaskBucket(GroupedTask* _tasks, size_t _count)
			: tasks(_tasks)
			, count(_count)
		{
		}
	};
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	
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

		void RunSubtasksAndYield(TaskGroup::Type taskGroup, fixed_array<TaskBucket>& buckets);

	public:

		FiberContext();

		template<class TTask>
		void RunSubtasksAndYield(TaskGroup::Type taskGroup, const TTask* taskArray, size_t count);

		template<class TTask>
		void RunAsync(TaskGroup::Type taskGroup, TTask* taskArray, uint32 count);

		void WaitGroupAndYield(TaskGroup::Type group);

		void Reset();

	public:

		// Active task attached to this fiber
		TaskDesc currentTask;

		// Active task status
		FiberTaskStatus::Type taskStatus;

		// Active task group
		TaskGroup::Type currentGroup;

		// Number of children fibers
		AtomicInt childrenFibersCount;

		// Parent fiber
		FiberContext* parentFiber;

		// Active thread context (null if fiber context is not executing now)
		ThreadContext * threadContext;

		// System Fiber
		Fiber fiber;

		// Prevent false sharing between threads
		uint8 cacheline[64];
	};


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct ThreadState
	{
		enum Type
		{
			ALIVE,
			EXIT,
		};
	};
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct GroupedTask
	{
		FiberContext* parentFiber;
		TaskGroup::Type group;
		TaskDesc desc;

		GroupedTask()
			: group(TaskGroup::GROUP_UNDEFINED)
			, parentFiber(nullptr)
		{}

		GroupedTask(TaskDesc& _desc, TaskGroup::Type _group)
			: group(_group)
			, desc(_desc)
			, parentFiber(nullptr)
		{}
	};
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Thread (Scheduler fiber) context
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct ThreadContext
	{
		// pointer to task manager
		TaskScheduler* taskScheduler;

		// thread
		Thread thread;

		// scheduler fiber
		Fiber schedulerFiber;

		// task queue awaiting execution
		ConcurrentQueueLIFO<GroupedTask> queue;

		// new task was arrived to queue event
		Event hasNewTasksEvent;

		// whether thread is alive
		AtomicInt state;

		// Temporary buffer
		std::vector<GroupedTask> descBuffer;

		// prevent false sharing between threads
		uint8 cacheline[64];

		ThreadContext();
		~ThreadContext();
	};


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Task scheduler
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class TaskScheduler
	{
		friend class FiberContext;

		struct GroupStats
		{
			AtomicInt inProgressTaskCount;
			Event allDoneEvent;

			GroupStats();
		};

		// Thread index for new task
		AtomicInt roundRobinThreadIndex;

		// Threads created by task manager
		uint32 threadsCount;
		ThreadContext threadContext[MT_MAX_THREAD_COUNT];

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

		FiberContext* RequestFiberContext(const GroupedTask& task);
		void ReleaseFiberContext(FiberContext* fiberExecutionContext);

		void RunTasksImpl(TaskGroup::Type taskGroup, fixed_array<TaskBucket>& buckets, FiberContext * parentFiber);
		void RestoreAwaitingTasks(TaskGroup::Type taskGroup);

		void RunTasksImpl(TaskGroup::Type taskGroup, TaskDesc* taskDescArr, size_t count, FiberContext * parentFiber);

		static void ThreadMain( void* userData );
		static void FiberMain( void* userData );
		static bool ExecuteTask (ThreadContext& threadContext, FiberContext* fiberContext);

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
		bool DistibuteDescriptions(TaskGroup::Type group, TTask* taskArray, fixed_array<GroupedTask>& descriptions, fixed_array<TaskBucket>& buckets) const
		{
			size_t index = 0;

			for (size_t bucketIndex = 0; (bucketIndex < buckets.size()) && (index < descriptions.size()); ++bucketIndex)
			{
				size_t bucketStartIndex = index;

				for (size_t i = bucketIndex; i < descriptions.size(); i += buckets.size())
				{
					TaskDesc desc(TTask::TaskFunction, (void*)(&taskArray[i]));
					descriptions[index++] = GroupedTask(desc, group);
				}

				buckets[bucketIndex] = TaskBucket(&descriptions[bucketStartIndex], index - bucketStartIndex);
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

        fixed_array<GroupedTask> buffer(ALLOCATE_ON_STACK(GroupedTask, count), count);

        size_t bucketCount = Min(threadsCount, count);
        fixed_array<TaskBucket>	buckets(ALLOCATE_ON_STACK(TaskBucket, bucketCount), bucketCount);

        DistibuteDescriptions(group, taskArray, buffer, buckets);
        RunTasksImpl(group, buckets, nullptr);
    }

    template<class TTask>
    void FiberContext::RunSubtasksAndYield(TaskGroup::Type taskGroup, const TTask* taskArray, size_t count)
    {
        ASSERT(threadContext, "ThreadContext is NULL");
        ASSERT(count < threadContext->descBuffer.size(), "Buffer overrun!")

        size_t threadsCount = threadContext->taskScheduler->GetWorkerCount();

        fixed_array<GroupedTask> buffer(&threadContext->descBuffer.front(), count);

        size_t bucketCount = Min(threadsCount, count);
        fixed_array<TaskBucket>	buckets(ALLOCATE_ON_STACK(TaskBucket, bucketCount), bucketCount);

        threadContext->taskScheduler->DistibuteDescriptions(taskGroup, taskArray, buffer, buckets);
        RunSubtasksAndYield(taskGroup, buckets);
    }

    template<class TTask>
    void FiberContext::RunAsync(TaskGroup::Type taskGroup, TTask* taskArray, uint32 count)
    {
        ASSERT(threadContext, "ThreadContext is NULL");
        ASSERT(threadContext->taskScheduler->IsWorkerThread(), "Can't use RunAsync outside Task. Use TaskScheduler.RunAsync() instead.");

        TaskScheduler& scheduler = *(threadContext->taskScheduler);

        fixed_array<GroupedTask> buffer(&threadContext->descBuffer.front(), count);

        size_t bucketCount = Min(scheduler.GetWorkerCount(), count);
        fixed_array<TaskBucket>	buckets(ALLOCATE_ON_STACK(TaskBucket, bucketCount), bucketCount);

        scheduler.DistibuteDescriptions(taskGroup, taskArray, buffer, buckets);
        scheduler.RunTasksImpl(taskGroup, buckets, nullptr);
    }



	#define TASK_METHODS(TASK_TYPE) static void TaskFunction(MT::FiberContext& fiberContext, void* userData) \
	                                {                                                                    \
	                                    TASK_TYPE* task = static_cast<TASK_TYPE*>(userData);             \
	                                    task->Do(fiberContext);                                          \
	                                }                                                                    \


}
