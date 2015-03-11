#pragma once

#include "Platform.h"
#include "ConcurrentQueue.h"

#define MT_MAX_THREAD_COUNT (32)
#define MT_SCHEDULER_STACK_SIZE (16384)

namespace MT
{

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



	typedef void (MT_CALL_CONV *TTaskEntryPoint)(void* userData);

	struct TaskDesc
	{
		TTaskEntryPoint taskFunc;
		void* userData;

		TaskDesc()
			: taskFunc(nullptr)
			, userData(nullptr)
		{

		}

		TaskDesc(TTaskEntryPoint _taskEntry, void* _userData)
			: taskFunc(_taskEntry)
			, userData(_userData)
		{
		}
	};

	struct ThreadContext
	{
		MT::Thread thread;
		MT::Fiber schedulerFiber;
		MT::ConcurrentQueue<MT::TaskDesc> queue[TaskGroup::COUNT];
		MT::Event* queueEmptyEvent[TaskGroup::COUNT];

		MT::Event hasNewEvents;
		MT::TaskGroup::Type activeGroup;

		ThreadContext();
	};


	struct ThreadGroupEvents
	{
		MT::Event threadQueueEmpty[MT_MAX_THREAD_COUNT];

		ThreadGroupEvents();
	};


	class TaskManager
	{
		uint32 newTaskThreadIndex;

		int32 threadsCount;
		ThreadContext threadContext[MT_MAX_THREAD_COUNT];
		ThreadGroupEvents groupDoneEvents[TaskGroup::COUNT];


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
				context.hasNewEvents.Signal();
			}
		}

		bool WaitGroup(MT::TaskGroup::Type group, uint32 milliseconds);

		static uint32 MT_CALL_CONV SchedulerThreadMain( void* userData );
		static void ExecuteTask (MT::TaskDesc & taskDesc);

	};
}