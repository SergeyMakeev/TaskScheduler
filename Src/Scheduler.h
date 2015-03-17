#pragma once

#include "Platform.h"
#include "ConcurrentQueueLIFO.h"
#include "Containers.h"

#ifdef Yield
#undef Yield
#endif

namespace MT
{

	const uint32 MT_MAX_THREAD_COUNT = 8;
	const uint32 MT_MAX_FIBERS_COUNT = 128;
	const uint32 MT_SCHEDULER_STACK_SIZE = 131072;
	const uint32 MT_FIBER_STACK_SIZE = 16384;

	//
	// Task group
	//
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


	struct TaskDesc;
	struct ThreadContext;
	struct FiberContext;

	class TaskScheduler;

	typedef void (*TTaskEntryPoint)(FiberContext & context, void* userData);

	//
	// Fiber task status
	//
	// Task can be completed for several reasons.
	// For example task was done or someone call Yield from the Task body.
	namespace FiberTaskStatus
	{
		enum Type
		{
			UNKNOWN = 0,
			RUNNED = 1,
			FINISHED = 2,
			AWAITING = 3,
		};
	}

	//
	// Fiber context
	//
	// Context passed to fiber main function
	struct FiberContext
	{
		// pointer to active task attached to this fiber
		TaskDesc * currentTask;

		// current group
		TaskGroup::Type currentGroup;

		// active thread context
		ThreadContext * threadContext;

		// active task status
		FiberTaskStatus::Type taskStatus;

		// Number of subtask fiber spawned
		AtomicInt subtaskFibersCount;

		// Pointer to system Fiber
		Fiber fiber;

		FiberContext();

	private:
		// prevent false sharing between threads
		uint8 cacheline[64];

		void RunSubtasksAndYield(TaskGroup::Type taskGroup, TaskDesc * taskDescArr, size_t count);

	public:
		template<class TTask>
		void RunSubtasksAndYield(TaskGroup::Type taskGroup, const TTask* taskArray, size_t count)
		{
			ASSERT(threadContext, "ThreadContext is NULL");

			threadContext->descBuffer.resize(count);

			TaskDesc* buffer = threadContext->descBuffer.begin();
			TaskScheduler::GenerateDescriptions(taskArray, buffer, count);

			RunSubtasksAndYield(taskGroup, buffer, count);
		}

		template<class TTask>
		void RunAsync(TaskGroup::Type group, TTask* taskArray, uint32 count)
		{
			ASSERT(threadContext, "ThreadContext is NULL");

			ASSERT(threadContext->taskScheduler->IsWorkerThread(), "Can't use RunAsync outside Task. Use TaskScheduler.RunAsync() instead.");

			TaskDesc* buffer = ALLOCATE_ON_STACK(TaskDesc, count);
			TaskScheduler::GenerateDescriptions(taskArray, buffer, count);
			threadContext->taskScheduler->RunTasksImpl(group, buffer, count, nullptr);
		}


		void WaitGroupAndYield(TaskGroup::Type group);
	};

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
			, parentTask(nullptr)
			, fiberContext(nullptr)
		//	, taskGroup(TaskGroup::GROUP_UNDEFINED)
		{

		}

		TaskDesc(TTaskEntryPoint _taskEntry, void* _userData)
			: taskFunc(_taskEntry)
			, userData(_userData)
			, parentTask(nullptr)
			, fiberContext(nullptr)
//			, taskGroup(TaskGroup::GROUP_UNDEFINED)
		{
		}

	private:

		friend struct FiberContext;
		friend class TaskScheduler;

		// Execution context. Not valid until scheduler attach fiber to task
		FiberContext* fiberContext;

		// Parent task pointer. Valid only for subtask
		TaskDesc* parentTask;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Base class for any Task
	struct Task
	{
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
		TaskGroup::Type group;
		TaskDesc desc;

		GroupedTask() : group(TaskGroup::GROUP_UNDEFINED) {}
		GroupedTask(TaskDesc& _desc, TaskGroup::Type _group) : desc(_desc), group(_group) {}
	};
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	typedef stack_array<TaskDesc, 4096> TaskDescBuffer;
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//
	// Thread (Scheduler fiber) context
	//
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
		TaskDescBuffer descBuffer;

		// prevent false sharing between threads
		uint8 cacheline[64];

		ThreadContext();
		~ThreadContext();
	};


	#define MAX_TASK_BATCH_COUNT 128

	//
	// Task scheduler
	//
	class TaskScheduler
	{
		friend struct FiberContext;

		// Thread index for new task
		AtomicInt roundRobinThreadIndex;

		// Threads created by task manager
		int32 threadsCount;
		ThreadContext threadContext[MT_MAX_THREAD_COUNT];

		// Per group events that is completed
		Event groupIsDoneEvents[TaskGroup::COUNT];

		AtomicInt groupInProgressTaskCount[TaskGroup::COUNT];

		//Task awaiting group through FiberContext::WaitGroupAndYield call
		ConcurrentQueueLIFO<TaskDesc> waitTaskQueues[TaskGroup::COUNT];


		// Fibers pool
		ConcurrentQueueLIFO<FiberContext*> availableFibers;

		// Fibers context
		FiberContext fiberContext[MT_MAX_FIBERS_COUNT];

		FiberContext* RequestFiberContext();
		void ReleaseFiberContext(FiberContext* fiberExecutionContext);

		bool PrepareTaskDescription(GroupedTask& task);
		void ReleaseTaskDescription(TaskDesc& description);

		void RestoreAwaitingTasks(TaskGroup::Type taskGroup);

		void RunTasksImpl(TaskGroup::Type taskGroup, TaskDesc* taskDescArr, size_t count, TaskDesc * parentTask);

		static void ThreadMain( void* userData );
		static void FiberMain( void* userData );
		static bool ExecuteTask (ThreadContext& context, const TaskDesc & taskDesc);

		template<class TTask>
		static bool GenerateDescriptions(TTask* taskArray, TaskDesc* descriptions, size_t count)
		{
			for (size_t i = 0; i < count; ++i)
				descriptions[i] = TaskDesc(TTask::TaskFunction, (void*)(&taskArray[i]));

			return count > 0;
		}

	public:

		TaskScheduler();
		~TaskScheduler();

		template<class TTask>
		void RunAsync(TaskGroup::Type group, TTask* taskArray, uint32 count)
		{
			ASSERT(!IsWorkerThread(), "Can't use RunAsync inside Task. Use FiberContext.RunAsync() instead.");

			TaskDesc* buffer = ALLOCATE_ON_STACK(TaskDesc, count);
			GenerateDescriptions(taskArray, buffer, count);
			RunTasksImpl(group, buffer, count, nullptr);
		}

		bool WaitGroup(TaskGroup::Type group, uint32 milliseconds);
		bool WaitAll(uint32 milliseconds);

		bool IsEmpty();

		int32 GetWorkerCount() const;

		bool IsWorkerThread() const;
	};


	#define TASK_METHODS(TASK_TYPE) static void TaskFunction(MT::FiberContext& fiberContext, void* userData) \
	                                {                                                                        \
	                                    TASK_TYPE* task = static_cast<TASK_TYPE*>(userData);                 \
	                                    task->Do(fiberContext);                                              \
	                                }                                                                        \


}