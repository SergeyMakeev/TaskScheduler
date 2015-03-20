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
		, lastActiveFiberContext(nullptr)
	{
	}

	ThreadContext::~ThreadContext()
	{
	}


	FiberContext::FiberContext()
		: taskStatus(FiberTaskStatus::UNKNOWN)
		, currentGroup(TaskGroup::GROUP_UNDEFINED)
		, childrenFibersCount(0)
		, parentFiber(nullptr)
		, threadContext(nullptr)
	{
	}

	void FiberContext::SetStatus(FiberTaskStatus::Type _taskStatus)
	{
		ASSERT(threadContext, "Sanity check failed");
		ASSERT(threadContext->thread.IsCurrentThread(), "You can change task status only from owner thread");
		taskStatus = _taskStatus;
	}

	FiberTaskStatus::Type FiberContext::GetStatus() const
	{
		return taskStatus;
	}


	void FiberContext::SetThreadContext(ThreadContext * _threadContext)
	{
		if (_threadContext)
		{
			_threadContext->lastActiveFiberContext = this;
		}

		threadContext = _threadContext;
	}

	ThreadContext* FiberContext::GetThreadContext()
	{
		return threadContext;
	}

	void FiberContext::Reset()
	{
		ASSERT(childrenFibersCount.Get() == 0, "Can't release fiber with active children fibers");

		currentGroup = TaskGroup::GROUP_UNDEFINED;
		currentTask = TaskDesc();
		parentFiber = nullptr;
		threadContext = nullptr;
	}

	void FiberContext::WaitGroupAndYield(TaskGroup::Type group)
	{
		ASSERT(threadContext, "Sanity check failed!");
		ASSERT(threadContext->taskScheduler->IsWorkerThread(), "Can't use WaitGroupAndYield outside Task. Use TaskScheduler.WaitGroup() instead.");
		ASSERT(threadContext->thread.IsCurrentThread(), "Thread context sanity check failed");

		VERIFY(group != currentGroup, "Can't wait the same group. Deadlock detected!", return);
		VERIFY(group < TaskGroup::COUNT, "Invalid group!", return);

		ConcurrentQueueLIFO<FiberContext*> & groupQueue = threadContext->taskScheduler->waitTaskQueues[group];

		// Change status
		taskStatus = FiberTaskStatus::AWAITING_GROUP;

		// Add current fiber to awaiting queue
		groupQueue.Push(this);

		Fiber & schedulerFiber = threadContext->schedulerFiber;

		// Yielding, so reset thread context
		threadContext = nullptr;

		//switch to scheduler
		Fiber::SwitchTo(fiber, schedulerFiber);
	}

	void FiberContext::RunSubtasksAndYield(TaskGroup::Type taskGroup, fixed_array<TaskBucket>& buckets)
	{
		ASSERT(threadContext, "Sanity check failed!");
		ASSERT(taskGroup < TaskGroup::COUNT, "Sanity check failed!");
		ASSERT(threadContext->taskScheduler->IsWorkerThread(), "Can't use RunSubtasksAndYield outside Task. Use TaskScheduler.WaitGroup() instead.");
		ASSERT(threadContext->thread.IsCurrentThread(), "Thread context sanity check failed");

		// add to scheduler
		threadContext->taskScheduler->RunTasksImpl(taskGroup, buckets, this);

		//
		ASSERT(threadContext->thread.IsCurrentThread(), "Thread context sanity check failed");

		// Change status
		taskStatus = FiberTaskStatus::AWAITING_CHILD;

		Fiber & schedulerFiber = threadContext->schedulerFiber;

		// Yielding, so reset thread context
		threadContext = nullptr;

		//switch to scheduler
		Fiber::SwitchTo(fiber, schedulerFiber);
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

	FiberContext* TaskScheduler::RequestFiberContext(const GroupedTask& task)
	{
		FiberContext *fiberContext = nullptr;
		if (!availableFibers.TryPop(fiberContext))
		{
			ASSERT(false, "Fibers pool is empty");
		}

		fiberContext->currentTask = task.desc;
		fiberContext->currentGroup = task.group;
		fiberContext->parentFiber = task.parentFiber;
		return fiberContext;
	}

	void TaskScheduler::ReleaseFiberContext(FiberContext* fiberContext)
	{
		ASSERT(fiberContext != nullptr, "Can't release nullptr Fiber");
		fiberContext->Reset();
		availableFibers.Push(fiberContext);
	}


	void TaskScheduler::RestoreAwaitingTasks(TaskGroup::Type taskGroup)
	{
		ConcurrentQueueLIFO<FiberContext*> & groupQueue = waitTaskQueues[taskGroup];
		//TODO: move awaiting tasks into execution thread queues
	}

	FiberContext* TaskScheduler::ExecuteTask(ThreadContext& threadContext, FiberContext* fiberContext)
	{
		ASSERT(threadContext.thread.IsCurrentThread(), "Thread context sanity check failed");

		ASSERT(fiberContext, "Invalid fiber context");
		ASSERT(fiberContext->currentTask.IsValid(), "Invalid task");
		ASSERT(fiberContext->currentGroup < TaskGroup::COUNT, "Invalid task group");

		// Set actual thread context to fiber
		fiberContext->SetThreadContext(&threadContext);

		// Update task status
		fiberContext->SetStatus(FiberTaskStatus::RUNNED);

		ASSERT(fiberContext->GetThreadContext()->thread.IsCurrentThread(), "Thread context sanity check failed");

		// Run current task code
		Fiber::SwitchTo(threadContext.schedulerFiber, fiberContext->fiber);

		// If task was done
		FiberTaskStatus::Type taskStatus = fiberContext->GetStatus();
		if (taskStatus == FiberTaskStatus::FINISHED)
		{
			TaskGroup::Type taskGroup = fiberContext->currentGroup;
			ASSERT(taskGroup < TaskGroup::COUNT, "Invalid group.");

			// Update group status
			int groupTaskCount = threadContext.taskScheduler->groupStats[taskGroup].inProgressTaskCount.Dec();
			ASSERT(groupTaskCount >= 0, "Sanity check failed!");
			if (groupTaskCount == 0)
			{
				// Restore awaiting tasks
				threadContext.taskScheduler->RestoreAwaitingTasks(taskGroup);
				threadContext.taskScheduler->groupStats[taskGroup].allDoneEvent.Signal();
			}

			// Update total task count
			groupTaskCount = threadContext.taskScheduler->allGroupStats.inProgressTaskCount.Dec();
			ASSERT(groupTaskCount >= 0, "Sanity check failed!");
			if (groupTaskCount == 0)
			{
				// Notify all tasks in all group finished
				threadContext.taskScheduler->allGroupStats.allDoneEvent.Signal();
			}

			FiberContext* parentFiberContext = fiberContext->parentFiber;
			if (parentFiberContext != nullptr)
			{
				int childrenFibersCount = parentFiberContext->childrenFibersCount.Dec();
				ASSERT(childrenFibersCount >= 0, "Sanity check failed!");

				if (childrenFibersCount == 0)
				{
					// This is a last subtask. Restore parent task
#if FIBER_DEBUG

					int ownerThread = parentFiberContext->fiber.GetOwnerThread();
					FiberTaskStatus::Type parentTaskStatus = parentFiberContext->GetStatus();
					ThreadContext * parentThreadContext = parentFiberContext->GetThreadContext();
					int fiberUsageCounter = parentFiberContext->fiber.GetUsageCounter();
					ASSERT(fiberUsageCounter == 0, "Parent fiber in invalid state");

					ownerThread;
					parentTaskStatus;
					parentThreadContext;
					fiberUsageCounter;
#endif

					ASSERT(threadContext.thread.IsCurrentThread(), "Thread context sanity check failed");
					ASSERT(parentFiberContext->GetThreadContext() == nullptr, "Inactive parent should not have a valid thread context");

					// WARNING!! Thread context can changed here! Set actual current thread context.
					parentFiberContext->SetThreadContext(&threadContext);

					ASSERT(parentFiberContext->GetThreadContext()->thread.IsCurrentThread(), "Thread context sanity check failed");

					// All subtasks is done.
					// Exiting and return parent fiber to scheduler
					return parentFiberContext;
				} else
				{
					// Other subtasks still exist
					// Exiting
					return nullptr;
				}
			} else
			{
				// Task is finished and no parent task
				// Exiting
				return nullptr;
			}
		}

		ASSERT(taskStatus != FiberTaskStatus::RUNNED, "Incorrect task status")
		return nullptr;
	}


	void TaskScheduler::FiberMain(void* userData)
	{
		FiberContext& fiberContext = *(FiberContext*)(userData);

		for(;;)
		{
			ASSERT(fiberContext.currentTask.IsValid(), "Invalid task in fiber context");
			ASSERT(fiberContext.currentGroup < TaskGroup::COUNT, "Invalid task group");
			ASSERT(fiberContext.GetThreadContext(), "Invalid thread context");
			ASSERT(fiberContext.GetThreadContext()->thread.IsCurrentThread(), "Thread context sanity check failed");

			fiberContext.currentTask.taskFunc( fiberContext, fiberContext.currentTask.userData );

			fiberContext.SetStatus(FiberTaskStatus::FINISHED);

			Fiber::SwitchTo(fiberContext.fiber, fiberContext.GetThreadContext()->schedulerFiber);
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
				// There is a new task

				FiberContext* fiberContext = context.taskScheduler->RequestFiberContext(task);
				ASSERT(fiberContext, "Can't get execution context from pool");
				ASSERT(fiberContext->currentTask.IsValid(), "Sanity check failed");


				while(fiberContext)
				{
					// prevent invalid fiber resume from child tasks, before ExecuteTask is done
					fiberContext->childrenFibersCount.Inc();

					FiberContext* parentFiber = ExecuteTask(context, fiberContext);

					FiberTaskStatus::Type taskStatus = fiberContext->GetStatus();

					//release guard
					int childrenFibersCount = fiberContext->childrenFibersCount.Dec();

					// Can drop fiber context - task is finished
					if (taskStatus == FiberTaskStatus::FINISHED)
					{
						ASSERT( childrenFibersCount == 0, "Sanity check failed");

						context.taskScheduler->ReleaseFiberContext(fiberContext);

						// If parent fiber is exist transfer flow control to parent fiber, if parent fiber is null, exit
						fiberContext = parentFiber;
					} else 
					{
						ASSERT( childrenFibersCount >= 0, "Sanity check failed");

						// No subtasks here and status is not finished, this mean all subtasks already finished before parent return from ExecuteTask
						if (childrenFibersCount == 0)
						{
							ASSERT(parentFiber == nullptr, "Sanity check failed");
						} else
						{
							// If subtasks still exist, drop current task execution. task will be resumed when last subtask finished
							break;
						}

						// If task is in await state drop execution. task will be resumed when RestoreAwaitingTasks called
						if (taskStatus == FiberTaskStatus::AWAITING_GROUP)
						{
							break;
						}
					}
				} //while(fiberContext)

			} else
			{
				// Queue is empty
				// TODO: can try to steal tasks from other threads
				context.hasNewTasksEvent.Wait(2000);
			}

		} // main thread loop

	}


	void TaskScheduler::RunTasksImpl(TaskGroup::Type taskGroup, fixed_array<TaskBucket>& buckets, FiberContext * parentFiber)
	{
		ASSERT(taskGroup < TaskGroup::COUNT, "Invalid group.");

		size_t count = 0;
		for (size_t i = 0; i < buckets.size(); ++i)
		{
			count += buckets[i].count;
		}

		if (parentFiber)
		{
			parentFiber->childrenFibersCount.Add((uint32)count);
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

			for (size_t taskIndex = 0; taskIndex < bucket.count; taskIndex++)
			{
				bucket.tasks[taskIndex].parentFiber = parentFiber;
			}

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

}
