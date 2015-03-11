#pragma once

#include "Platform.h"
#include "ConcurrentQueue.h"

#define MT_MAX_THREAD_COUNT (4)
#define MT_MAX_FIBERS_COUNT (128)
#define MT_SCHEDULER_STACK_SIZE (16384)
#define MT_FIBER_STACK_SIZE (16384)

#ifdef Yield
#undef Yield
#endif

namespace MT
{
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

			COUNT = 3
		};
	}


	struct TaskDesc;
	struct ThreadContext;
	class TaskManager;

	typedef void (MT_CALL_CONV *TTaskEntryPoint)(MT::ThreadContext & context, void* userData);

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
			YIELDED = 2,
			FINISHED = 3,
		};
	}

	//
	// Fiber context
	//
	// Context passed to fiber main function
	struct FiberContext
	{
		// pointer to active task attached to this fiber
		TaskDesc * activeTask;

		// active thread context
		ThreadContext * activeContext;

		// active task status
		FiberTaskStatus::Type taskStatus;

		FiberContext()
			: activeTask(nullptr)
			, activeContext(nullptr)
			, taskStatus(FiberTaskStatus::UNKNOWN)
		{}
	};

	//
	// Fiber descriptor
	//
	// Hold pointer to fiber and pointer to fiber context
	//
	struct FiberDesc
	{
		MT::Fiber fiber;
		MT::FiberContext * fiberContext;

		FiberDesc(MT::Fiber _fiber, MT::FiberContext * _fiberContext)
			: fiber(_fiber)
			, fiberContext(_fiberContext)
		{}

		bool IsValid() const
		{
			return (fiber != nullptr && fiberContext != nullptr);
		}

		static FiberDesc Empty()
		{
			return FiberDesc(nullptr, nullptr);
		}
	};

	//
	// Task description
	//
	struct TaskDesc
	{
		//Active fiber description. Not valid until scheduler attach fiber to task
		FiberDesc activeFiber;

		//Task entry point
		TTaskEntryPoint taskFunc;

		//Task user data (task context)
		void* userData;

		TaskDesc()
			: taskFunc(nullptr)
			, userData(nullptr)
			, activeFiber(FiberDesc::Empty())
		{

		}

		TaskDesc(TTaskEntryPoint _taskEntry, void* _userData)
			: taskFunc(_taskEntry)
			, userData(_userData)
			, activeFiber(FiberDesc::Empty())
		{
		}
	};


	//
	// Thread (Scheduler fiber) context
	//
	struct ThreadContext
	{
		//Pointer to task manager
		MT::TaskManager* taskManager;

		//Thread
		MT::Thread thread;

		//Scheduler fiber
		MT::Fiber schedulerFiber;

		//Task queues. Divided by different groups
		MT::ConcurrentQueue<MT::TaskDesc> queue[TaskGroup::COUNT];

		//Queue is empty events
		MT::Event* queueEmptyEvent[TaskGroup::COUNT];

		//New task was arrived event
		MT::Event hasNewTasksEvent;

		//Active task group to schedule
		MT::TaskGroup::Type activeGroup;

		ThreadContext();

		void Yield();
	};


	//
	// Task manager
	//
	class TaskManager
	{

		// Transposed thread queue events. Used inside WaitGroup implementation.
		struct ThreadGroupEvents
		{
			MT::Event threadQueueEmpty[MT_MAX_THREAD_COUNT];
			
			ThreadGroupEvents();
		};

		// Thread index to new task
		uint32 newTaskThreadIndex;


		// Threads created by task manager
		int32 threadsCount;
		ThreadContext threadContext[MT_MAX_THREAD_COUNT];

		//Per group events that the work in group completed in all threads
		ThreadGroupEvents groupDoneEvents[TaskGroup::COUNT];

		//Fibers pool
		MT::ConcurrentQueue<FiberDesc> freeFibers;

		//Fibers context
		MT::FiberContext fiberContext[MT_MAX_FIBERS_COUNT];

		FiberDesc RequestFiber();
		void ReleaseFiber(FiberDesc fiberDesc);


	public:

		TaskManager();
		~TaskManager();


		template<typename T, int size>
		void RunTasks(TaskGroup::Type taskGroup, const T(&taskDesc)[size])
		{
			for (int i = 0; i < size; i++)
			{
				ThreadContext & context = threadContext[newTaskThreadIndex];
				newTaskThreadIndex = (newTaskThreadIndex + 1) % (uint32)threadsCount;

				//TODO: can be write more effective implementation here, just split to threads before submitting tasks to queue
				context.queue[taskGroup].Push(taskDesc[i]);
				context.queueEmptyEvent[taskGroup]->Reset();
				context.hasNewTasksEvent.Signal();
			}
		}

		bool WaitGroup(MT::TaskGroup::Type group, uint32 milliseconds);

		static uint32 MT_CALL_CONV ThreadMain( void* userData );
		static void MT_CALL_CONV FiberMain(void* userData);

		static void ExecuteTask (MT::ThreadContext& context, MT::TaskDesc & taskDesc);

	};
}