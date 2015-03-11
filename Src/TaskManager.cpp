#include "TaskManager.h"



namespace MT
{
	SchedulerThreadContext::SchedulerThreadContext()
		: hasNewEvents(EventReset::MANUAL, true)
		, thread(nullptr)
	{
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
		}

		for (int32 i = 0; i < threadsCount; i++)
		{
			MT::ResumeThread(threadContext[i].thread);
		}
	}

	TaskManager::~TaskManager()
	{
	}

	uint32 TaskManager::SchedulerThreadMain( void* userData )
	{
		SchedulerThreadContext * context = (SchedulerThreadContext *)userData;
		context->schedulerFiber = MT::ConvertCurrentThreadToFiber();

		for(;;)
		{
			MT::TaskDesc taskDesc;
			if (context->queue.TryPop(taskDesc))
			{
				//there is a new task
				if (taskDesc.taskFunc != nullptr)
				{
					taskDesc.taskFunc(taskDesc.userData);
				}
			} else
			{
				//TODO: can try to steal tasks from other threads

				//all tasks was done. wait 2 seconds
				context->hasNewEvents.Reset();
				context->hasNewEvents.Wait(2000);
			}
		}

		return 0;
	}

}