#include "TaskManager.h"
#include "Assert.h"



namespace MT
{
	ThreadContext::ThreadContext()
		: hasNewTasksEvent(EventReset::MANUAL, true)
		, taskManager(nullptr)
		, thread(nullptr)
		, activeGroup(MT::TaskGroup::GROUP_0)
		, schedulerFiber(nullptr)
	{
		for (int i = 0; i < TaskGroup::COUNT; i++)
		{
			queueEmptyEvent[i] = nullptr;
		}
	}


	void ThreadContext::Yield()
	{
		MT::SwitchToFiber(schedulerFiber);
	}

	TaskManager::ThreadGroupEvents::ThreadGroupEvents()
	{
		for (int i = 0; i < MT_MAX_THREAD_COUNT; i++)
		{
			threadQueueEmpty[i].Create(MT::EventReset::MANUAL, true);
		}
	}


	TaskManager::TaskManager()
		: newTaskThreadIndex(0)
	{

		//query number of processor
		threadsCount = MT::GetNumberOfProcessors() - 2;
		if (threadsCount <= 0)
		{
			threadsCount = 1;
		}

		if (threadsCount > MT_MAX_THREAD_COUNT)
		{
			threadsCount = MT_MAX_THREAD_COUNT;
		}

		//create fiber pool
		for (int32 i = 0; i < MT_MAX_FIBERS_COUNT; i++)
		{
			MT::Fiber fiber = MT::CreateFiber(MT_FIBER_STACK_SIZE, FiberMain, &fiberContext[i]);
			freeFibers.Push(FiberDesc(fiber, &fiberContext[i]));
		}

		//create worker thread pool
		for (int32 i = 0; i < threadsCount; i++)
		{
			threadContext[i].taskManager = this;
			threadContext[i].thread = MT::CreateSuspendedThread(MT_SCHEDULER_STACK_SIZE, ThreadMain, &threadContext[i] );
			MT::SetThreadProcessor(threadContext[i].thread, i);

			//
			for (int j = 0; j < TaskGroup::COUNT; j++)
			{
				threadContext[i].queueEmptyEvent[j] = &groupDoneEvents[j].threadQueueEmpty[i];
			}
		}

		//run worker threads
		for (int32 i = 0; i < threadsCount; i++)
		{
			MT::ResumeThread(threadContext[i].thread);
		}
	}

	TaskManager::~TaskManager()
	{
	}

	FiberDesc TaskManager::RequestFiber()
	{
		FiberDesc fiber = FiberDesc::Empty();
		if (!freeFibers.TryPop(fiber))
		{
			ASSERT(false, "Fibers pool is empty");
		}
		return fiber;
	}

	void TaskManager::ReleaseFiber(FiberDesc fiberDesc)
	{
		freeFibers.Push(fiberDesc);
	}


	void TaskManager::ExecuteTask (MT::ThreadContext& context, MT::TaskDesc & taskDesc)
	{
		if (taskDesc.taskFunc != nullptr)
		{
			taskDesc.activeFiber = context.taskManager->RequestFiber();
			ASSERT(taskDesc.activeFiber.IsValid(), "Can't get fiber");

			taskDesc.activeFiber.fiberContext->activeTask = &taskDesc;
			taskDesc.activeFiber.fiberContext->activeContext = &context;

			taskDesc.activeFiber.fiberContext->taskStatus = FiberTaskStatus::RUNNED;
			MT::SwitchToFiber(taskDesc.activeFiber.fiber);

			if (taskDesc.activeFiber.fiberContext->taskStatus == FiberTaskStatus::FINISHED)
			{
				//task was done
				//releasing task fiber
				context.taskManager->ReleaseFiber(taskDesc.activeFiber);
				taskDesc.activeFiber = FiberDesc::Empty();
			} else
			{
				//task was yielded, save task and fiber to yielded queue
				taskDesc.activeFiber.fiberContext->taskStatus = FiberTaskStatus::YIELDED;
			}

		}
	}


	void TaskManager::FiberMain(void* userData)
	{
		MT::FiberContext& context = *(MT::FiberContext*)(userData);

		for(;;)
		{
			ASSERT(context.activeTask, "Invalid task in fiber context");
			context.activeTask->taskFunc( *context.activeContext, context.activeTask->userData );
			context.taskStatus = FiberTaskStatus::FINISHED;
			MT::SwitchToFiber(context.activeContext->schedulerFiber);
		}

	}


	uint32 TaskManager::ThreadMain( void* userData )
	{
		ThreadContext& context = *(ThreadContext*)(userData);
		ASSERT(context.taskManager, "Task manager must be not null!");
		context.schedulerFiber = MT::ConvertCurrentThreadToFiber();

		for(;;)
		{
			MT::TaskDesc taskDesc;
			if (context.queue[context.activeGroup].TryPop(taskDesc))
			{
				//there is a new task
				ExecuteTask(context, taskDesc);
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
						ExecuteTask(context, taskDesc);
						break;
					}
				}

				//TODO: can try to steal tasks from other threads
				if (groupFound == false)
				{
					//all tasks was done. wait 2 seconds
					context.hasNewTasksEvent.Reset();
					context.hasNewTasksEvent.Wait(2000);
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