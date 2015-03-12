#include "Scheduler.h"
#include "Assert.h"


//TODO: Split to files. One file - one class.

namespace MT
{
	ThreadContext::ThreadContext()
		: hasNewTasksEvent(EventReset::MANUAL, true)
		, taskScheduler(nullptr)
		, thread(nullptr)
		, schedulerFiber(nullptr)
	{
	}

	void ThreadContext::Yield()
	{
		MT::SwitchToFiber(schedulerFiber);
	}


	TaskScheduler::TaskScheduler()
		: roundRobinThreadIndex(0)
	{

		//query number of processor
		threadsCount = MT::GetNumberOfHardwareThreads() - 2;
		if (threadsCount <= 0)
		{
			threadsCount = 1;
		}

		if (threadsCount > MT_MAX_THREAD_COUNT)
		{
			threadsCount = MT_MAX_THREAD_COUNT;
		}

		// create fiber pool
		for (int32 i = 0; i < MT_MAX_FIBERS_COUNT; i++)
		{
			MT::Fiber fiber = MT::CreateFiber(MT_FIBER_STACK_SIZE, FiberMain, &fiberContext[i]);
			availableFibers.Push(FiberExecutionContext(fiber, &fiberContext[i]));
		}

		// create group done events
		for (int32 i = 0; i < TaskGroup::COUNT; i++)
		{
			groupIsDoneEvents[i].Create(MT::EventReset::MANUAL, true);
			groupCurrentlyRunningTaskCount[i].Set(0);
		}

		// create worker thread pool
		for (int32 i = 0; i < threadsCount; i++)
		{
			threadContext[i].taskScheduler = this;
			threadContext[i].thread = MT::CreateSuspendedThread(MT_SCHEDULER_STACK_SIZE, ThreadMain, &threadContext[i] );

			// bind thread to processor
			MT::SetThreadProcessor(threadContext[i].thread, i);
		}

		// run worker threads
		for (int32 i = 0; i < threadsCount; i++)
		{
			MT::ResumeThread(threadContext[i].thread);
		}
	}

	TaskScheduler::~TaskScheduler()
	{
	}

	MT::FiberExecutionContext TaskScheduler::RequestFiber()
	{
		MT::FiberExecutionContext fiber = MT::FiberExecutionContext::Empty();
		if (!availableFibers.TryPop(fiber))
		{
			ASSERT(false, "Fibers pool is empty");
		}
		return fiber;
	}

	void TaskScheduler::ReleaseFiber(MT::FiberExecutionContext fiberExecutionContext)
	{
		availableFibers.Push(fiberExecutionContext);
	}


	void TaskScheduler::ExecuteTask (MT::ThreadContext& context, MT::TaskDesc & taskDesc)
	{
		if (taskDesc.taskFunc != nullptr)
		{
			taskDesc.activeFiber = context.taskScheduler->RequestFiber();
			ASSERT(taskDesc.activeFiber.IsValid(), "Can't get fiber");

			taskDesc.activeFiber.fiberContext->activeTask = &taskDesc;
			taskDesc.activeFiber.fiberContext->activeContext = &context;

			taskDesc.activeFiber.fiberContext->taskStatus = FiberTaskStatus::RUNNED;
			MT::SwitchToFiber(taskDesc.activeFiber.fiber);

			if (taskDesc.activeFiber.fiberContext->taskStatus == FiberTaskStatus::FINISHED)
			{
				//task was done
				int groupTaskCount = context.taskScheduler->groupCurrentlyRunningTaskCount[taskDesc.taskGroup].Dec();
				ASSERT(groupTaskCount >= 0, "Sanity check failed!");
				if (groupTaskCount == 0)
				{
					context.taskScheduler->groupIsDoneEvents[taskDesc.taskGroup].Signal();
				}

				//releasing task fiber
				context.taskScheduler->ReleaseFiber(taskDesc.activeFiber);
				taskDesc.activeFiber = MT::FiberExecutionContext::Empty();
			} else
			{
				//task was yielded, save task and fiber to yielded queue
			}

		}
	}


	void TaskScheduler::FiberMain(void* userData)
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


	uint32 TaskScheduler::ThreadMain( void* userData )
	{
		ThreadContext& context = *(ThreadContext*)(userData);
		ASSERT(context.taskScheduler, "Task scheduler must be not null!");
		context.schedulerFiber = MT::ConvertCurrentThreadToFiber();

		for(;;)
		{
			MT::TaskDesc taskDesc;
			if (context.queue.TryPop(taskDesc))
			{
				//there is a new task
				ExecuteTask(context, taskDesc);
			} else
			{
				//TODO: can try to steal tasks from other threads
				//all tasks was done. wait 2 seconds
				context.hasNewTasksEvent.Reset();
				context.hasNewTasksEvent.Wait(2000);
			}
		}

		return 0;
	}

	bool TaskScheduler::WaitGroup(MT::TaskGroup::Type group, uint32 milliseconds)
	{
		return groupIsDoneEvents[group].Wait(milliseconds);
	}

	bool TaskScheduler::WaitAll(uint32 milliseconds)
	{
		return MT::WaitForMultipleEvents(&groupIsDoneEvents[0], ARRAY_SIZE(groupIsDoneEvents), milliseconds);
	}

}