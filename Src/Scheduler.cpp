#include "Scheduler.h"
#include "Assert.h"


//TODO: Split to files. One file - one class.

namespace MT
{
	ThreadContext::ThreadContext()
		: hasNewTasksEvent(EventReset::AUTOMATIC, true)
		, state(ThreadState::ALIVE)
		, taskScheduler(nullptr)
	{
	}

	ThreadContext::~ThreadContext()
	{
	}


	FiberContext::FiberContext()
		: currentTask(nullptr)
		, threadContext(nullptr)
		, subtaskFibersCount(0)
		, taskStatus(FiberTaskStatus::UNKNOWN)
	{
	}

	bool FiberContext::WaitGroupAndYield(MT::TaskGroup::Type group, uint32 milliseconds)
	{
		return threadContext->taskScheduler->WaitGroup(group, milliseconds);
	}

	bool FiberContext::WaitAllAndYield(uint32 milliseconds)
	{
		return threadContext->taskScheduler->WaitAll(milliseconds);
	}

	void FiberContext::RunSubtasksAndYield(MT::TaskGroup::Type taskGroup, MT::TaskDesc * taskDescArr, size_t count)
	{
		ASSERT(threadContext, "Sanity check failed!");

		ASSERT(currentTask, "Sanity check failed!");
		ASSERT(taskGroup < TaskGroup::COUNT, "Sanity check failed!");

		// ATTENTION !
		// copy current task description to stack.
		//  pointer to parentTask alive until all child task finished
		MT::TaskDesc parentTask = *currentTask;

		ASSERT(threadContext->thread.IsCurrentThread(), "Thread context sanity check failed");

		//add subtask to scheduler
		threadContext->taskScheduler->RunTasksImpl(taskGroup, taskDescArr, count, &parentTask);

		//
		ASSERT(threadContext->thread.IsCurrentThread(), "Thread context sanity check failed");

		//switch to scheduler

		threadContext->schedulerFiber.SwitchTo();
	}


	TaskScheduler::TaskScheduler()
		: roundRobinThreadIndex(0)
	{

		//query number of processor
		threadsCount = MT::Thread::GetNumberOfHardwareThreads() - 2;
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
			MT::FiberContext& context = fiberContext[i];
			context.fiber.Create(MT_FIBER_STACK_SIZE, FiberMain, &context);
			availableFibers.Push( &context );
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
			threadContext[i].thread.Start( MT_SCHEDULER_STACK_SIZE, ThreadMain, &threadContext[i] );
		}

	}

	TaskScheduler::~TaskScheduler()
	{
		for (int32 i = 0; i < threadsCount; i++)
		{
			threadContext[i].state.Set(ThreadState::EXIT);
			threadContext[i].hasNewTasksEvent.Signal();
		}

		for (int32 i = 0; i < threadsCount; i++)
		{
			threadContext[i].thread.Stop();
		}
	}

	MT::FiberContext* TaskScheduler::RequestFiberContext()
	{
		MT::FiberContext *fiber = nullptr;
		if (!availableFibers.TryPop(fiber))
		{
			ASSERT(false, "Fibers pool is empty");
		}
		return fiber;
	}

	void TaskScheduler::ReleaseFiberContext(MT::FiberContext* fiberContext)
	{
		ASSERT(fiberContext != nullptr, "Can't release nullptr Fiber");

		fiberContext->currentGroup = TaskGroup::GROUP_UNDEFINED;
		fiberContext->currentTask = nullptr;

		availableFibers.Push(fiberContext);
	}


	bool TaskScheduler::ExecuteTask (MT::ThreadContext& context, const MT::TaskDesc & taskDesc)
	{
		bool canDropExecutionContext = false;

		MT::TaskDesc taskInProgress = taskDesc;
		for(int iteration = 0;;iteration++)
		{
			ASSERT(taskInProgress.taskFunc != nullptr, "Invalid task function pointer");
			ASSERT(taskInProgress.fiberContext, "Invalid execution context.");

			taskInProgress.fiberContext->threadContext = &context;

			// update task status
			taskInProgress.fiberContext->taskStatus = FiberTaskStatus::RUNNED;

			ASSERT(context.thread.IsCurrentThread(), "Thread context sanity check failed");
			ASSERT(taskInProgress.fiberContext->threadContext->thread.IsCurrentThread(), "Thread context sanity check failed");

			// run current task code
			taskInProgress.fiberContext->fiber.SwitchTo();

			// if task was done
			if (taskInProgress.fiberContext->taskStatus == FiberTaskStatus::FINISHED)
			{
				TaskGroup::Type taskGroup = taskInProgress.fiberContext->currentGroup;

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
					context.taskScheduler->ReleaseTaskDescription(taskInProgress);
				}


				//
				if (taskInProgress.parentTask != nullptr)
				{
					int subTasksCount = taskInProgress.parentTask->fiberContext->subtaskFibersCount.Dec();
					ASSERT(subTasksCount >= 0, "Sanity check failed!");

					if (subTasksCount == 0)
					{
						// this is a last subtask. restore parent task

						MT::TaskDesc * parent = taskInProgress.parentTask;
						
						ASSERT(context.thread.IsCurrentThread(), "Thread context sanity check failed");

						// WARNING!! Thread context can changed here! Set actual current thread context.
						parent->fiberContext->threadContext = &context;

						ASSERT(parent->fiberContext->threadContext->thread.IsCurrentThread(), "Thread context sanity check failed");

						// copy parent to current task.
						// can't just use pointer, because parent pointer is pointer on fiber stack
						taskInProgress = *parent;

						taskInProgress.fiberContext->currentTask = &taskInProgress;
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
			ASSERT(context.currentGroup < TaskGroup::COUNT, "Invalid task group");
			ASSERT(context.threadContext, "Invalid thread context");
			ASSERT(context.threadContext->thread.IsCurrentThread(), "Thread context sanity check failed");

			context.currentTask->taskFunc( context, context.currentTask->userData );

			context.taskStatus = FiberTaskStatus::FINISHED;
			context.threadContext->schedulerFiber.SwitchTo();
		}

	}


	void TaskScheduler::ThreadMain( void* userData )
	{
		ThreadContext& context = *(ThreadContext*)(userData);
		ASSERT(context.taskScheduler, "Task scheduler must be not null!");
		context.schedulerFiber.CreateFromCurrentThread();

		while(context.state.Get() != ThreadState::EXIT)
		{
			MT::GroupedTask task;
			if (context.queue.TryPop(task))
			{
				//there is a new task

				context.taskScheduler->PrepareTaskDescription(task);
				ASSERT(task.desc.fiberContext, "Can't get execution context from pool");
				ASSERT(task.desc.fiberContext->currentTask->taskFunc, "Sanity check failed");

				for(;;)
				{
					// prevent invalid fiber resume from child tasks, before ExecuteTask is done
					task.desc.fiberContext->subtaskFibersCount.Inc();
					bool canDropContext = ExecuteTask(context, task.desc);
					int subtaskCount = task.desc.fiberContext->subtaskFibersCount.Dec();
					ASSERT(subtaskCount >= 0, "Sanity check failed");

					bool taskIsFinished = (task.desc.fiberContext->taskStatus == FiberTaskStatus::FINISHED);

					if (canDropContext)
					{
						context.taskScheduler->ReleaseTaskDescription(task.desc);
						ASSERT(task.desc.fiberContext == nullptr, "Sanity check failed");
					}

					// if subtasks still exist, drop current task execution. current task will be resumed when last subtask finished
					if (subtaskCount > 0 || taskIsFinished)
					{
						break;
					}

					// no subtasks and status is not finished, this mean all subtasks already finished before parent return from ExecuteTask
					// continue task execution
				}

				if (task.desc.fiberContext)
					task.desc.fiberContext->currentTask = nullptr;
				

			} 
			else
			{
				//TODO: can try to steal tasks from other threads
				context.hasNewTasksEvent.Wait(2000);
			}
		}
	}


	void MT::TaskScheduler::RunTasksImpl(TaskGroup::Type taskGroup, MT::TaskDesc* taskDescArr, size_t count, MT::TaskDesc * parentTask)
	{
		ASSERT(taskGroup < TaskGroup::COUNT, "Invalid group.");

		if (parentTask)
		{
			parentTask->fiberContext->subtaskFibersCount.Add((uint32)count);
		}

		for (size_t i = count - 1; i != (size_t)-1; i--)
		{
			int bucketIndex = roundRobinThreadIndex.Inc() % threadsCount;
			ThreadContext & context = threadContext[bucketIndex];
			
			//TODO: can be write more effective implementation here, just split to threads BEFORE submitting tasks to queue
			MT::GroupedTask task(taskDescArr[i], taskGroup);
			task.desc.parentTask = parentTask;

			groupIsDoneEvents[taskGroup].Reset();
			groupCurrentlyRunningTaskCount[taskGroup].Inc();
			
			context.queue.Push(task);
			
			context.hasNewTasksEvent.Signal();
		}
	}

	bool TaskScheduler::WaitGroup(MT::TaskGroup::Type group, uint32 milliseconds)
	{
		VERIFY(IsWorkerThread() == false, "Can't use WaitGroup inside Task. Use MT::FiberContext.WaitGroupAndYield() instead.", return false);

		return groupIsDoneEvents[group].Wait(milliseconds);
	}

	bool TaskScheduler::WaitAll(uint32 milliseconds)
	{
		VERIFY(IsWorkerThread() == false, "Can't use WaitAll inside Task. Use MT::FiberContext.WaitAllAndYield() instead.", return false);

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

	bool TaskScheduler::IsWorkerThread() const
	{
		for (int i = 0; i < MT_MAX_THREAD_COUNT; i++)
		{
			if (threadContext[i].thread.IsCurrentThread())
			{
				return true;
			}
		}
		return false;
	}

	void TaskScheduler::ReleaseTaskDescription(MT::TaskDesc& description)
	{
		ReleaseFiberContext(description.fiberContext);
		description.fiberContext = nullptr;
	}

	bool TaskScheduler::PrepareTaskDescription(MT::GroupedTask& task)
	{
		task.desc.fiberContext = RequestFiberContext();
		task.desc.fiberContext->currentTask = &task.desc;
		task.desc.fiberContext->currentGroup = task.group;
		return task.desc.fiberContext != nullptr;
	}

}