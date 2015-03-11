#include "TaskManager.h"



namespace MT
{
	ThreadContext::ThreadContext()
		: hasNewEvents(EventReset::MANUAL, true)
		, thread(nullptr)
		, activeGroup(MT::TaskGroup::GROUP_0)
	{
		for (int i = 0; i < TaskGroup::COUNT; i++)
		{
			queueEmptyEvent[i] = nullptr;
		}
	}

	ThreadGroupEvents::ThreadGroupEvents()
	{
		for (int i = 0; i < MT_MAX_THREAD_COUNT; i++)
		{
			threadQueueEmpty[i].Create(MT::EventReset::MANUAL, true);
		}
	}

	TaskManager::TaskManager()
		: newTaskThreadIndex(0)
	{
		threadsCount = MT::GetNumberOfProcessors() - 2;
		if (threadsCount <= 0)
		{
			threadsCount = 1;
		}

		for (int32 i = 0; i < threadsCount; i++)
		{
			threadContext[i].thread = MT::CreateSuspendedThread(MT_SCHEDULER_STACK_SIZE, SchedulerThreadMain, &threadContext[i] );
			MT::SetThreadProcessor(threadContext[i].thread, i);

			for (int j = 0; j < TaskGroup::COUNT; j++)
			{
				threadContext[i].queueEmptyEvent[j] = &groupDoneEvents[j].threadQueueEmpty[i];
			}
		}

		for (int32 i = 0; i < threadsCount; i++)
		{
			MT::ResumeThread(threadContext[i].thread);
		}
	}

	TaskManager::~TaskManager()
	{
	}

	void TaskManager::ExecuteTask (MT::TaskDesc & taskDesc)
	{
		if (taskDesc.taskFunc != nullptr)
		{
			taskDesc.taskFunc(taskDesc.userData);
		}
	}

	uint32 TaskManager::SchedulerThreadMain( void* userData )
	{
		ThreadContext& context = *(ThreadContext*)(userData);
		context.schedulerFiber = MT::ConvertCurrentThreadToFiber();

		for(;;)
		{
			MT::TaskDesc taskDesc;
			if (context.queue[context.activeGroup].TryPop(taskDesc))
			{
				//there is a new task
				ExecuteTask(taskDesc);
			} else
			{
				context.queueEmptyEvent[context.activeGroup]->Signal();

				//Try to find new task group to execute
				bool groupFound = false;
				for (int j = 1; j < MT::TaskGroup::COUNT; j++)
				{
					uint32 groupIndex = ((uint32)context.activeGroup + 1) % MT::TaskGroup::COUNT;
					if (context.queue[groupIndex].TryPop(taskDesc))
					{
						groupFound = true;
						context.activeGroup = (MT::TaskGroup::Type)groupIndex;
						ExecuteTask(taskDesc);
						break;
					}
				}

				//TODO: can try to steal tasks from other threads
				if (groupFound == false)
				{
					//all tasks was done. wait 2 seconds
					context.hasNewEvents.Reset();
					context.hasNewEvents.Wait(2000);
				}

			}
		}

		return 0;
	}

	bool TaskManager::WaitGroup(MT::TaskGroup::Type group, uint32 milliseconds)
	{
		return MT::WaitForMultipleEvents(&groupDoneEvents[group].threadQueueEmpty[0], threadsCount, milliseconds);
	}

}