// The MIT License (MIT)
// 
// 	Copyright (c) 2015 Sergey Makeev, Vadim Slyusarev
// 
// 	Permission is hereby granted, free of charge, to any person obtaining a copy
// 	of this software and associated documentation files (the "Software"), to deal
// 	in the Software without restriction, including without limitation the rights
// 	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// 	copies of the Software, and to permit persons to whom the Software is
// 	furnished to do so, subject to the following conditions:
// 
//  The above copyright notice and this permission notice shall be included in
// 	all copies or substantial portions of the Software.
// 
// 	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// 	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// 	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// 	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// 	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// 	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// 	THE SOFTWARE.

#include <MTScheduler.h>

namespace MT
{

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
			threadContext[i].SetThreadIndex(i);
			threadContext[i].taskScheduler = this;
			threadContext[i].thread.Start( MT_SCHEDULER_STACK_SIZE, ThreadMain, &threadContext[i] );
		}
	}

	TaskScheduler::~TaskScheduler()
	{
		for (uint32 i = 0; i < threadsCount; i++)
		{
			threadContext[i].state.Set(internal::ThreadState::EXIT);
			threadContext[i].hasNewTasksEvent.Signal();
		}

		for (uint32 i = 0; i < threadsCount; i++)
		{
			threadContext[i].thread.Stop();
		}
	}

	FiberContext* TaskScheduler::RequestFiberContext(internal::GroupedTask& task)
	{
		FiberContext *fiberContext = task.awaitingFiber;
		if (fiberContext)
		{
			task.awaitingFiber = nullptr;
			return fiberContext;
		}

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

	FiberContext* TaskScheduler::ExecuteTask(internal::ThreadContext& threadContext, FiberContext* fiberContext)
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
				threadContext.RestoreAwaitingTasks(taskGroup);
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
					internal::ThreadContext * parentThreadContext = parentFiberContext->GetThreadContext();
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


	bool TaskScheduler::StealTask(internal::ThreadContext& threadContext, internal::GroupedTask & task)
	{
		// Try to steal tasks from random worker thread
		uint32 workersCount = threadContext.taskScheduler->GetWorkerCount();
		if (workersCount <= 1)
		{
			return false;
		}

		uint32 victimIndex = threadContext.random.Get() % workersCount;
		if (victimIndex == threadContext.workerIndex)
		{
			victimIndex = victimIndex++;
			victimIndex = victimIndex % workersCount;
		}

		internal::ThreadContext& victimContext = threadContext.taskScheduler->threadContext[victimIndex];
		return victimContext.queue.TryPop(task);
	}

	void TaskScheduler::ThreadMain( void* userData )
	{
		internal::ThreadContext& context = *(internal::ThreadContext*)(userData);
		ASSERT(context.taskScheduler, "Task scheduler must be not null!");
		context.schedulerFiber.CreateFromThread(context.thread);

		while(context.state.Get() != internal::ThreadState::EXIT)
		{
			internal::GroupedTask task;
			if (context.queue.TryPop(task) || StealTask(context, task) )
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
				// Queue is empty and stealing attempt failed
				// Wait new events
				context.hasNewTasksEvent.Wait(2000);
			}

		} // main thread loop
	}

	void TaskScheduler::RunTasksImpl(WrapperArray<internal::TaskBucket>& buckets, FiberContext * parentFiber, bool restoredFromAwaitState)
	{
		// Reset counter to initial value
		int taskCountInGroup[TaskGroup::COUNT];
		for (size_t i = 0; i < TaskGroup::COUNT; ++i)
		{
			taskCountInGroup[i] = 0;
		}

		// Set parent fiber pointer
		// Calculate the number of tasks per group
		// Calculate total number of tasks
		size_t count = 0;
		for (size_t i = 0; i < buckets.Size(); ++i)
		{
			internal::TaskBucket& bucket = buckets[i];
			for (size_t taskIndex = 0; taskIndex < bucket.count; taskIndex++)
			{
				internal::GroupedTask & task = bucket.tasks[taskIndex];

				ASSERT(task.group < TaskGroup::COUNT, "Invalid group.");

				task.parentFiber = parentFiber;
				taskCountInGroup[task.group]++;
			}
			count += bucket.count;
		}

		// Increments child fibers count on parent fiber
		if (parentFiber)
		{
			parentFiber->childrenFibersCount.Add((uint32)count);
		}

		if (restoredFromAwaitState == false)
		{
			// Increments all task in progress counter
			allGroupStats.allDoneEvent.Reset();
			allGroupStats.inProgressTaskCount.Add((uint32)count);

			// Increments task in progress counters (per group)
			for (size_t i = 0; i < TaskGroup::COUNT; ++i)
			{
				int groupTaskCount = taskCountInGroup[i];
				if (groupTaskCount > 0)
				{
					groupStats[i].allDoneEvent.Reset();
					groupStats[i].inProgressTaskCount.Add((uint32)groupTaskCount);
				}
			}
		} else
		{
			// If task's restored from await state, counters already in correct state
		}

		// Add to thread queue
		for (size_t i = 0; i < buckets.Size(); ++i)
		{
			int bucketIndex = roundRobinThreadIndex.Inc() % threadsCount;
			internal::ThreadContext & context = threadContext[bucketIndex];

			internal::TaskBucket& bucket = buckets[i];

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
