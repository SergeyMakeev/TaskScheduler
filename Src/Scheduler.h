#pragma once

#include "Platform.h"
#include "ConcurrentQueueLIFO.h"

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

	typedef void (MT_CALL_CONV *TTaskEntryPoint)(MT::FiberContext & context, void* userData);

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

		// active thread context
		ThreadContext * threadContext;

		// active task status
		FiberTaskStatus::Type taskStatus;

		// Number of subtask fiber spawned
		MT::AtomicInt subtaskFibersCount;

		// prevent false sharing between threads
		uint8 cacheline[64];

		FiberContext();

		void RunSubtasksAndYield(const MT::TaskDesc * taskDescArr, uint32 count);
	};

	//
	// Fiber execution context
	//
	// Hold pointer to fiber and pointer to fiber context
	//
	struct FiberExecutionContext
	{
		MT::Fiber fiber;
		MT::FiberContext * fiberContext;

		FiberExecutionContext(MT::Fiber _fiber, MT::FiberContext * _fiberContext)
			: fiber(_fiber)
			, fiberContext(_fiberContext)
		{}

		bool IsValid() const
		{
			return (fiber != nullptr && fiberContext != nullptr);
		}

		static FiberExecutionContext Empty()
		{
			return FiberExecutionContext(nullptr, nullptr);
		}
	};

	//
	// Task description
	//
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
			, taskGroup(TaskGroup::GROUP_UNDEFINED)
			, executionContext(FiberExecutionContext::Empty())
		{

		}

		TaskDesc(TTaskEntryPoint _taskEntry, void* _userData)
			: taskFunc(_taskEntry)
			, userData(_userData)
			, parentTask(nullptr)
			, taskGroup(TaskGroup::GROUP_UNDEFINED)
			, executionContext(FiberExecutionContext::Empty())
		{
		}


		TaskGroup::Type GetGroup() const
		{
			return taskGroup;
		}

	private:

		friend struct FiberContext;
		friend class TaskScheduler;

		// Execution context. Not valid until scheduler attach fiber to task
		FiberExecutionContext executionContext;

		// Parent task pointer. Valid only for subtask
		TaskDesc* parentTask;

		//
		TaskGroup::Type taskGroup;

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

	//
	// Thread (Scheduler fiber) context
	//
	struct ThreadContext
	{
		// pointer to task manager
		MT::TaskScheduler* taskScheduler;

		// thread
		MT::Thread thread;

		// scheduler fiber
		MT::Fiber schedulerFiber;

		// task queue awaiting execution
		MT::ConcurrentQueueLIFO<MT::TaskDesc> queue;

		// new task was arrived to queue event
		MT::Event hasNewTasksEvent;

		uint32 debugThreadId;

		// whether thread is alive
		MT::AtomicInt state;

		// prevent false sharing between threads
		uint8 cacheline[64];

		ThreadContext();
		~ThreadContext();
	};


	//
	// Task scheduler
	//
	class TaskScheduler
	{
		friend struct MT::FiberContext;

		// Thread index for new task
		MT::AtomicInt roundRobinThreadIndex;

		// Threads created by task manager
		int32 threadsCount;
		ThreadContext threadContext[MT_MAX_THREAD_COUNT];

		// Per group events that is completed
		MT::Event groupIsDoneEvents[TaskGroup::COUNT];

		MT::AtomicInt groupCurrentlyRunningTaskCount[TaskGroup::COUNT];

		// Fibers pool
		MT::ConcurrentQueueLIFO<FiberExecutionContext> availableFibers;

		// Fibers context
		MT::FiberContext fiberContext[MT_MAX_FIBERS_COUNT];

		FiberExecutionContext RequestExecutionContext();
		void ReleaseExecutionContext(const MT::FiberExecutionContext & fiberExecutionContext);

		void RunTasksImpl(TaskGroup::Type taskGroup, const MT::TaskDesc * taskDescArr, uint32 count, MT::TaskDesc * parentTask);


		static uint32 MT_CALL_CONV ThreadMain( void* userData );
		static void MT_CALL_CONV FiberMain(void* userData);

		static bool ExecuteTask (MT::ThreadContext& context, const MT::TaskDesc & taskDesc);

	public:

		TaskScheduler();
		~TaskScheduler();

		void RunTasks(TaskGroup::Type taskGroup, const MT::TaskDesc * taskDescArr, uint32 count);

		bool WaitGroup(MT::TaskGroup::Type group, uint32 milliseconds);
		bool WaitAll(uint32 milliseconds);

		bool IsEmpty();

		int32 GetWorkerCount() const;


	};
}