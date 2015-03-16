#include "Scheduler.h"
#include "Assert.h"


//TODO: Split to files. One file - one class.

namespace MT
{
	ThreadContext::ThreadContext()
		: hasNewTasksEvent(EventReset::AUTOMATIC, true)
		, state(ThreadState::ALIVE)
		, taskScheduler(nullptr)
		, thread(nullptr)
		, threadId(0)
		, schedulerFiber(nullptr)
	{
	}

	ThreadContext::~ThreadContext()
	{
		ASSERT(thread == NULL, "Thread is not stopped!")
	}


	FiberContext::FiberContext()
		: currentTask(nullptr)
		, threadContext(nullptr)
		, subtaskFibersCount(0)
		, taskStatus(FiberTaskStatus::UNKNOWN)
	{
	}

	void FiberContext::RunSubtasksAndYield(const MT::TaskDesc * taskDescArr, uint32 count)
	{
		ASSERT(threadContext, "Sanity check failed!");

		ASSERT(currentTask, "Sanity check failed!");
		ASSERT(currentTask->taskGroup < TaskGroup::COUNT, "Sanity check failed!");

		// ATTENTION !
		// copy current task description to stack.
		//  pointer to parentTask alive until all child task finished
		MT::TaskDesc parentTask = *currentTask;

		ASSERT(threadContext->threadId == MT::GetCurrentThreadId(), "Thread context sanity check failed");

		//add subtask to scheduler
		threadContext->taskScheduler->RunTasksImpl(parentTask.taskGroup, taskDescArr, count, &parentTask);

		//
		ASSERT(threadContext->threadId == MT::GetCurrentThreadId(), "Thread context sanity check failed");

		//switch to scheduler
		MT::SwitchToFiber(threadContext->schedulerFiber);
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
			MT::Fiber fiber = MT::CreateFiber( MT_FIBER_STACK_SIZE, FiberMain, &fiberContext[i] );
			availableFibers.Push( FiberExecutionContext(fiber, &fiberContext[i]) );
		}

		// create group done events
		for (int32 i = 0; i < TaskGroup::COUNT; i++)
		{
			groupIsDoneEvents[i].Create( MT::EventReset::MANUAL, true );
			groupCurrentlyRunningTaskCount[i].Set(0);
		}

		// create worker thread pool
		for (int32 i = 0; i < threadsCount; i++)
		{
			threadContext[i].taskScheduler = this;
			threadContext[i].thread = MT::CreateSuspendedThread( MT_SCHEDULER_STACK_SIZE, ThreadMain, &threadContext[i] );

			// bind thread to processor
			MT::SetThreadProcessor(threadContext[i].thread, i);
		}

		// run worker threads
		for (int32 i = 0; i < threadsCount; i++)
		{
			MT::ResumeThread(threadContext[i].thread);
		}
	}

	static const int THREAD_CLOSE_TIMEOUT_MS = 200;

	TaskScheduler::~TaskScheduler()
	{
		for (int32 i = 0; i < threadsCount; i++)
		{
			threadContext[i].state.Set(ThreadState::EXIT);
			threadContext[i].hasNewTasksEvent.Signal();
		}

		for (int32 i = 0; i < threadsCount; i++)
		{
			CloseThread(threadContext[i].thread, THREAD_CLOSE_TIMEOUT_MS);
			threadContext[i].thread = NULL;
		}
	}

	MT::FiberExecutionContext TaskScheduler::RequestExecutionContext()
	{
		MT::FiberExecutionContext fiber = MT::FiberExecutionContext::Empty();
		if (!availableFibers.TryPop(fiber))
		{
			ASSERT(false, "Fibers pool is empty");
		}
		return fiber;
	}

	void TaskScheduler::ReleaseExecutionContext(const MT::FiberExecutionContext & fiberExecutionContext)
	{
		ASSERT(fiberExecutionContext.fiber != nullptr, "Invalid execution fiber in execution context");
		ASSERT(fiberExecutionContext.fiberContext != nullptr, "Invalid fiber context in execution context");
		availableFibers.Push(fiberExecutionContext);
	}


	bool TaskScheduler::ExecuteTask (MT::ThreadContext& context, const MT::TaskDesc & taskDesc)
	{
		bool canDropExecutionContext = false;

		MT::TaskDesc taskInProgress = taskDesc;
		for(int iteration = 0;;iteration++)
		{
			ASSERT(taskInProgress.taskFunc != nullptr, "Invalid task function pointer");
			ASSERT(taskInProgress.executionContext.fiberContext, "Invalid execution context.");

			taskInProgress.executionContext.fiberContext->threadContext = &context;

			// update task status
			taskInProgress.executionContext.fiberContext->taskStatus = FiberTaskStatus::RUNNED;

			ASSERT(context.threadId == MT::GetCurrentThreadId(), "Thread context sanity check failed");
			ASSERT(taskInProgress.executionContext.fiberContext->threadContext->threadId == MT::GetCurrentThreadId(), "Thread context sanity check failed");

			// run current task code
			MT::SwitchToFiber(taskInProgress.executionContext.fiber);

			// if task was done
			if (taskInProgress.executionContext.fiberContext->taskStatus == FiberTaskStatus::FINISHED)
			{
				TaskGroup::Type taskGroup = taskInProgress.taskGroup;

				ASSERT(taskGroup < TaskGroup::COUNT, "Invalid group.");

				//update group status
				int groupTaskCount = context.taskScheduler->groupCurrentlyRunningTaskCount[taskGroup].Dec();
				ASSERT(groupTaskCount >= 0, "Sanity check failed!");
				if (groupTaskCount == 0)
				{
					context.taskScheduler->groupIsDoneEvents[taskGroup].Signal();
				}

				//raise up releasing task fiber flag
				canDropExecutionContext = true;

				//
				if (iteration > 0)
				{
					taskInProgress.executionContext.fiberContext->currentTask = nullptr;
					context.taskScheduler->ReleaseExecutionContext(taskInProgress.executionContext);
					taskInProgress.executionContext = MT::FiberExecutionContext::Empty();
					ASSERT(taskInProgress.executionContext.fiber == nullptr, "Sanity check failed");
				}


				//
				if (taskInProgress.parentTask != nullptr)
				{
					int subTasksCount = taskInProgress.parentTask->executionContext.fiberContext->subtaskFibersCount.Dec();
					ASSERT(subTasksCount >= 0, "Sanity check failed!");

					if (subTasksCount == 0)
					{
						// this is a last subtask. restore parent task

						MT::TaskDesc * parent = taskInProgress.parentTask;

						ASSERT(context.threadId == MT::GetCurrentThreadId(), "Thread context sanity check failed");

						// WARNING!! Thread context can changed here! Set actual current thread context.
						parent->executionContext.fiberContext->threadContext = &context;

						ASSERT(parent->executionContext.fiberContext->threadContext->threadId == MT::GetCurrentThreadId(), "Thread context sanity check failed");

						// copy parent to current task.
						// can't just use pointer, because parent pointer is pointer on fiber stack
						taskInProgress = *parent;

						parent->executionContext.fiberContext->currentTask = &taskInProgress;

					} else
					{
						// subtask still not finished
						// exiting
						break;
					}
				} else
				{
					// no parent task
					// exiting
					break;
				}
			} else
			{
				// current task was yielded, due to subtask spawn
				// exiting
				break;
			}

		} // loop

		return canDropExecutionContext;
	}


	void TaskScheduler::FiberMain(void* userData)
	{
		MT::FiberContext& context = *(MT::FiberContext*)(userData);

		for(;;)
		{
			ASSERT(context.currentTask, "Invalid task in fiber context");
			ASSERT(context.currentTask->taskFunc, "Invalid task function");
			ASSERT(context.currentTask->taskGroup < TaskGroup::COUNT, "Invalid task group");
			ASSERT(context.threadContext, "Invalid thread context");
			ASSERT(context.threadContext->threadId == MT::GetCurrentThreadId(), "Thread context sanity check failed");

			context.currentTask->taskFunc( context, context.currentTask->userData );

			context.taskStatus = FiberTaskStatus::FINISHED;
			MT::SwitchToFiber(context.threadContext->schedulerFiber);
		}

	}


	uint32 TaskScheduler::ThreadMain( void* userData )
	{
		ThreadContext& context = *(ThreadContext*)(userData);
		ASSERT(context.taskScheduler, "Task scheduler must be not null!");
		context.threadId = MT::GetCurrentThreadId();
		context.schedulerFiber = MT::ConvertCurrentThreadToFiber();

		while(context.state.Get() != ThreadState::EXIT)
		{
			MT::TaskDesc taskDesc;
			if (context.queue.TryPop(taskDesc))
			{
				//there is a new task

				taskDesc.executionContext = context.taskScheduler->RequestExecutionContext();
				ASSERT(taskDesc.executionContext.IsValid(), "Can't get execution context from pool");

				taskDesc.executionContext.fiberContext->currentTask = &taskDesc;
				ASSERT(taskDesc.executionContext.fiberContext->currentTask->taskFunc, "Sanity check failed");

				for(;;)
				{
					// prevent invalid fiber resume from child tasks, before ExecuteTask is done
					taskDesc.executionContext.fiberContext->subtaskFibersCount.Inc();
					bool canDropContext = ExecuteTask(context, taskDesc);
					int subtaskCount = taskDesc.executionContext.fiberContext->subtaskFibersCount.Dec();
					ASSERT(subtaskCount >= 0, "Sanity check failed");

					bool taskIsFinished = (taskDesc.executionContext.fiberContext->taskStatus == FiberTaskStatus::FINISHED);

					if (canDropContext)
					{
						taskDesc.executionContext.fiberContext->currentTask = nullptr;
						context.taskScheduler->ReleaseExecutionContext(taskDesc.executionContext);
						taskDesc.executionContext = MT::FiberExecutionContext::Empty();
						ASSERT(taskDesc.executionContext.fiber == nullptr, "Sanity check failed");
					}

					// if subtasks still exist, drop current task execution. current task will be resumed when last subtask finished
					if (subtaskCount > 0 || taskIsFinished)
					{
						break;
					}

					// no subtasks and status is not finished, this mean all subtasks already finished before parent return from ExecuteTask
					// continue task execution
				}

				if (taskDesc.executionContext.fiberContext)
					taskDesc.executionContext.fiberContext->currentTask = nullptr;
				

			} 
			else
			{
				//TODO: can try to steal tasks from other threads
				context.hasNewTasksEvent.Wait(2000);
			}
		}

		return 0;
	}

	void TaskScheduler::RunTasksImpl(TaskGroup::Type taskGroup, const MT::TaskDesc * taskDescArr, uint32 count, MT::TaskDesc * parentTask)
	{
		ASSERT(taskGroup < TaskGroup::COUNT, "Invalid group.");

		if (parentTask)
		{
			parentTask->executionContext.fiberContext->subtaskFibersCount.Add(count);
		}

		int startIndex = ((int)count - 1);
		for (int i = startIndex; i >= 0; i--)
		{
			int bucketIndex = roundRobinThreadIndex.Inc() % threadsCount;
			ThreadContext & context = threadContext[bucketIndex];
			
			//TODO: can be write more effective implementation here, just split to threads BEFORE submitting tasks to queue
			MT::TaskDesc desc = taskDescArr[i];
			desc.taskGroup = taskGroup;
			desc.parentTask = parentTask;

			groupIsDoneEvents[taskGroup].Reset();
			groupCurrentlyRunningTaskCount[taskGroup].Inc();
			
			context.queue.Push(desc);
			
			context.hasNewTasksEvent.Signal();
		}
	}


	void TaskScheduler::RunAsync(TaskGroup::Type taskGroup, const MT::TaskDesc * taskDescArr, uint32 count)
	{
		RunTasksImpl(taskGroup, taskDescArr, count, nullptr);
	}


	bool TaskScheduler::WaitGroup(MT::TaskGroup::Type group, uint32 milliseconds)
	{
		VERIFY(IsWorkerThread(MT::GetCurrentThreadId()) == false, "Can't use WaitGroup inside Task. Use MT::FiberContext.WaitGroupAndYield() instead.", return false);

		return groupIsDoneEvents[group].Wait(milliseconds);
	}

	bool TaskScheduler::WaitAll(uint32 milliseconds)
	{
		VERIFY(IsWorkerThread(MT::GetCurrentThreadId()) == false, "Can't use WaitAll inside Task. Use MT::FiberContext.WaitAllAndYield() instead.", return false);

		return Event::WaitAll(&groupIsDoneEvents[0], ARRAY_SIZE(groupIsDoneEvents), milliseconds);
	}

	bool TaskScheduler::IsEmpty()
	{
		for (int i = 0; i < MT_MAX_THREAD_COUNT; i++)
		{
			if (!threadContext[i].queue.IsEmpty())
			{
				return false;
			}
		}
		return true;
	}

	int32 TaskScheduler::GetWorkerCount() const
	{
		return threadsCount;
	}

	bool TaskScheduler::IsWorkerThread(uint32 threadId) const
	{
		for (int i = 0; i < MT_MAX_THREAD_COUNT; i++)
		{
			if (threadContext[i].threadId == threadId)
			{
				return true;
			}
		}
		return false;
	}

}