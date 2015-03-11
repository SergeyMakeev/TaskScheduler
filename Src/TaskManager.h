#pragma once

#include "Platform.h"
#include "ConcurrentQueue.h"

#define MT_MAX_THREAD_COUNT (32)
#define MT_SCHEDULER_STACK_SIZE (16384)

namespace MT
{


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

	struct SchedulerThreadContext
	{
		MT::Thread thread;
		MT::Fiber schedulerFiber;
		MT::ConcurrentQueue<MT::TaskDesc> queue;
		MT::Event hasNewEvents;

		SchedulerThreadContext();
	};


	class TaskManager
	{
		uint32 newTaskThreadIndex;

		int32 threadsCount;
		SchedulerThreadContext threadContext[MT_MAX_THREAD_COUNT];

	public:

		TaskManager();
		~TaskManager();

		
		template<typename T, int size>
		void RunTasks(const T(&taskDesc)[size])
		{
			for (int i = 0; i < size; i++)
			{
				SchedulerThreadContext & context = threadContext[newTaskThreadIndex];
				newTaskThreadIndex = (newTaskThreadIndex + 1) % (uint32)threadsCount;

				//TODO: can be write more effective implementation here, just split to threads before submitting tasks to queue
				context.queue.Push(taskDesc[i]);
				context.hasNewEvents.Set();
			}
		}

		static uint32 MT_CALL_CONV SchedulerThreadMain( void* userData );

	};
}