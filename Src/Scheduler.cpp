#include "Scheduler.h"
//#include "Assert.h"


//TODO: Split to files. One file - one class.

namespace MT
{
	static const size_t TASK_BUFFER_CAPACITY = 4096;

	ThreadContext::ThreadContext()
		: taskScheduler(nullptr)
		, hasNewTasksEvent(EventReset::AUTOMATIC, true)
		, state(ThreadState::ALIVE)
		, descBuffer(TASK_BUFFER_CAPACITY)
	{
	}

	ThreadContext::~ThreadContext()
	{
	}


	FiberContext::FiberContext()
		: currentTask(nullptr)
		, currentGroup(TaskGroup::GROUP_UNDEFINED)
		, threadContext(nullptr)
		, taskStatus(FiberTaskStatus::UNKNOWN)
		, subtaskFibersCount(0)
	{
	}

	void FiberContext::WaitGroupAndYield(TaskGroup::Type group)
	{
		VERIFY(group != currentGroup, "Can't wait the same group. Deadlock detected!", return);
		VERIFY(group < TaskGroup::COUNT, "Invalid group!", return);
		ASSERT(threadContext->taskScheduler->IsWorkerThread(), "Can't use WaitGroup outside Task. Use TaskScheduler.WaitGroup() instead.");

		ConcurrentQueueLIFO<TaskDesc> & groupQueue = threadContext->taskScheduler->waitTaskQueues[group];

		//change status
		taskStatus = FiberTaskStatus::AWAITING;

		// copy current task to awaiting queue
		groupQueue.Push(*currentTask);

		//
		ASSERT(threadContext->thread.IsCurrentThread(), "Thread context sanity check failed");

		//switch to scheduler
		Fiber::SwitchTo(fiber, threadContext->schedulerFiber);
	}

	void FiberContext::RunSubtasksAndYield(TaskGroup::Type taskGroup, fixed_array<TaskBucket>& buckets)
	{
		ASSERT(threadContext, "Sanity check failed!");

		ASSERT(currentTask, "Sanity check failed!");
		ASSERT(taskGroup < TaskGroup::COUNT, "Sanity check failed!");

		// ATTENTION !
		// copy current task description to stack.
		//  pointer to parentTask alive until all child task finished
		TaskDesc parentTask = *currentTask;

		ASSERT(threadContext->thread.IsCurrentThread(), "Thread context sanity check failed");

		for (size_t bucketIndex = 0; bucketIndex < buckets.size(); ++bucketIndex)
		{
			TaskBucket& bucket = buckets[bucketIndex];
			for (size_t i = 0; i < bucket.count; ++i)
				bucket.tasks[i].desc.parentTask = &parentTask;
		}

		//add subtask to scheduler
		threadContext->taskScheduler->RunTasksImpl(taskGroup, buckets, &parentTask);

		//
		ASSERT(threadContext->thread.IsCurrentThread(), "Thread context sanity check failed");

		//switch to scheduler
		Fiber::SwitchTo(fiber, threadContext->schedulerFiber);
	}


	TaskScheduler::GroupStats::GroupStats()
	{
		inProgressTaskCount.Set(0);
		allDoneEvent.Create( EventReset::MANUAL, true );
	}

	TaskScheduler::TaskScheduler()
		: roundRobinThreadIndex(0)
	{

		//query number of processor
		threadsCount = Max(Thread::GetNumberOfHardwareThreads() - 2, 1);

		if (threadsCount > MT_MAX_THREAD_COUNT)
		{
			threadsCount = MT_MAX_THREAD_COUNT;
		}

		// create fiber pool
		for (uint32 i = 0; i < MT_MAX_FIBERS_COUNT; i++)
		{
			FiberContext& context = fiberContext[i];
			context.fiber.Create(MT_FIBER_STACK_SIZE, FiberMain, &context);
			availableFibers.Push( &context );
		}

		// create worker thread pool
		for (uint32 i = 0; i < threadsCount; i++)
		{
			threadContext[i].taskScheduler = this;
			threadContext[i].thread.Start( MT_SCHEDULER_STACK_SIZE, ThreadMain, &threadContext[i] );
		}

	}

	TaskScheduler::~TaskScheduler()
	{
		for (uint32 i = 0; i < threadsCount; i++)
		{
			threadContext[i].state.Set(ThreadState::EXIT);
			threadContext[i].hasNewTasksEvent.Signal();
		}

		for (uint32 i = 0; i < threadsCount; i++)
		{
			threadContext[i].thread.Stop();
		}
	}

	FiberContext* TaskScheduler::RequestFiberContext()
	{
		FiberContext *fiber = nullptr;
		if (!availableFibers.TryPop(fiber))
		{
			ASSERT(false, "Fibers pool is empty");
		}
		return fiber;
	}

	void TaskScheduler::ReleaseFiberContext(FiberContext* fiberContext)
	{
		ASSERT(fiberContext != nullptr, "Can't release nullptr Fiber");

		fiberContext->currentGroup = TaskGroup::GROUP_UNDEFINED;
		fiberContext->currentTask = nullptr;

		availableFibers.Push(fiberContext);
	}


	void TaskScheduler::RestoreAwaitingTasks(TaskGroup::Type taskGroup)
	{
		ConcurrentQueueLIFO<TaskDesc> & groupQueue = waitTaskQueues[taskGroup];
		//TODO: move awaiting tasks into execution thread queues
	}

	bool TaskScheduler::ExecuteTask (ThreadContext& context, const TaskDesc & taskDesc)
	{
		bool canDropExecutionContext = false;

		TaskDesc taskInProgress = taskDesc;
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
			Fiber::SwitchTo(context.schedulerFiber, taskInProgress.fiberContext->fiber);

			// if task was done
			if (taskInProgress.fiberContext->taskStatus == FiberTaskStatus::FINISHED)
			{
				TaskGroup::Type taskGroup = taskInProgress.fiberContext->currentGroup;

				ASSERT(taskGroup < TaskGroup::COUNT, "Invalid group.");

				//update group status
				int groupTaskCount = context.taskScheduler->groupStats[taskGroup].inProgressTaskCount.Dec();
				ASSERT(groupTaskCount >= 0, "Sanity check failed!");
				if (groupTaskCount == 0)
				{
					//restore awaiting tasks
					context.taskScheduler->RestoreAwaitingTasks(taskGroup);

					context.taskScheduler->groupStats[taskGroup].allDoneEvent.Signal();
				}

				groupTaskCount = context.taskScheduler->allGroupStats.inProgressTaskCount.Dec();
				ASSERT(groupTaskCount >= 0, "Sanity check failed!");
				if (groupTaskCount == 0)
				{
					//notify all tasks in all group finished
					context.taskScheduler->allGroupStats.allDoneEvent.Signal();
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

						TaskDesc * parent = taskInProgress.parentTask;

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
			} 
			else if (taskInProgress.fiberContext->taskStatus == FiberTaskStatus::AWAITING)
			{
				// current task was yielded, due to awaiting another task group
				// exiting
				break;
			}
			else if (taskInProgress.fiberContext->taskStatus == FiberTaskStatus::RUNNED)
			{
				// current task was yielded, due to subtask spawn
				// exiting
				break;
			}
			else
			{
				ASSERT(false, "State is not supperted. Undefined behaviour!")
			}
		} // loop

		return canDropExecutionContext;
	}


	void TaskScheduler::FiberMain(void* userData)
	{
		FiberContext& context = *(FiberContext*)(userData);

		for(;;)
		{
			ASSERT(context.currentTask, "Invalid task in fiber context");
			ASSERT(context.currentTask->taskFunc, "Invalid task function");
			ASSERT(context.currentGroup < TaskGroup::COUNT, "Invalid task group");
			ASSERT(context.threadContext, "Invalid thread context");
			ASSERT(context.threadContext->thread.IsCurrentThread(), "Thread context sanity check failed");

			context.currentTask->taskFunc( context, context.currentTask->userData );

			context.taskStatus = FiberTaskStatus::FINISHED;

			Fiber::SwitchTo(context.fiber, context.threadContext->schedulerFiber);
		}

	}


	void TaskScheduler::ThreadMain( void* userData )
	{
		ThreadContext& context = *(ThreadContext*)(userData);
		ASSERT(context.taskScheduler, "Task scheduler must be not null!");
		context.schedulerFiber.CreateFromThread(context.thread);

		while(context.state.Get() != ThreadState::EXIT)
		{
			GroupedTask task;
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
					bool taskIsAwait = (task.desc.fiberContext->taskStatus == FiberTaskStatus::AWAITING);

					if (canDropContext)
					{
						context.taskScheduler->ReleaseTaskDescription(task.desc);
						ASSERT(task.desc.fiberContext == nullptr, "Sanity check failed");
					}

					// if subtasks still exist, drop current task execution. current task will be resumed when last subtask finished
					if (subtaskCount > 0)
					{
						break;
					}

					//if task is finished or task is in await state drop execution
					if (taskIsFinished || taskIsAwait)
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


	void TaskScheduler::RunTasksImpl(TaskGroup::Type taskGroup, fixed_array<TaskBucket>& buckets, TaskDesc * parentTask)
	{
		ASSERT(taskGroup < TaskGroup::COUNT, "Invalid group.");

		size_t count = 0;

		for (size_t i = 0; i < buckets.size(); ++i)
			count += buckets[i].count;

		if (parentTask)
		{
			parentTask->fiberContext->subtaskFibersCount.Add((uint32)count);
		}

		for (size_t i = 0; i < buckets.size(); ++i)
		{
			int bucketIndex = roundRobinThreadIndex.Inc() % threadsCount;
			ThreadContext & context = threadContext[bucketIndex];

			TaskBucket& bucket = buckets[i];

			allGroupStats.allDoneEvent.Reset();
			allGroupStats.inProgressTaskCount.Add((uint32)bucket.count);

			groupStats[taskGroup].allDoneEvent.Reset();
			groupStats[taskGroup].inProgressTaskCount.Add((uint32)bucket.count);

			context.queue.PushRange(bucket.tasks, bucket.count);

			context.hasNewTasksEvent.Signal();

		}
	}

	bool TaskScheduler::WaitGroup(TaskGroup::Type group, uint32 milliseconds)
	{
		VERIFY(IsWorkerThread() == false, "Can't use WaitGroup inside Task. Use FiberContext.WaitGroupAndYield() instead.", return false);

		return groupStats[group].allDoneEvent.Wait(milliseconds);
	}

	bool TaskScheduler::WaitAll(uint32 milliseconds)
	{
		VERIFY(IsWorkerThread() == false, "Can't use WaitAll inside Task.", return false);

		return allGroupStats.allDoneEvent.Wait(milliseconds);
	}

	bool TaskScheduler::IsEmpty()
	{
		for (uint32 i = 0; i < MT_MAX_THREAD_COUNT; i++)
		{
			if (!threadContext[i].queue.IsEmpty())
			{
				return false;
			}
		}
		return true;
	}

	uint32 TaskScheduler::GetWorkerCount() const
	{
		return threadsCount;
	}

	bool TaskScheduler::IsWorkerThread() const
	{
		for (uint32 i = 0; i < MT_MAX_THREAD_COUNT; i++)
		{
			if (threadContext[i].thread.IsCurrentThread())
			{
				return true;
			}
		}
		return false;
	}

	void TaskScheduler::ReleaseTaskDescription(TaskDesc& description)
	{
		ReleaseFiberContext(description.fiberContext);
		description.fiberContext = nullptr;
	}

	bool TaskScheduler::PrepareTaskDescription(GroupedTask& task)
	{
		task.desc.fiberContext = RequestFiberContext();
		task.desc.fiberContext->currentTask = &task.desc;
		task.desc.fiberContext->currentGroup = task.group;
		return task.desc.fiberContext != nullptr;
	}

}
