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
#include <string.h> // for memset

namespace MT
{

#ifdef MT_INSTRUMENTED_BUILD
	TaskScheduler::TaskScheduler(uint32 workerThreadsCount, IProfilerEventListener* listener)
#else
	TaskScheduler::TaskScheduler(uint32 workerThreadsCount)
#endif
		: roundRobinThreadIndex(0)
		, startedThreadsCount(0)
	{

#ifdef MT_INSTRUMENTED_BUILD
		profilerEventListener = listener;
#endif

		if (workerThreadsCount != 0)
		{
			threadsCount.StoreRelaxed( MT::Clamp(workerThreadsCount, (uint32)1, (uint32)MT_MAX_THREAD_COUNT) );
		} else
		{
			//query number of processor
			threadsCount.StoreRelaxed( (uint32)MT::Clamp(Thread::GetNumberOfHardwareThreads(), 1, (int)MT_MAX_THREAD_COUNT) );
		}

		// create fiber pool (fibers with standard stack size)
		for (uint32 i = 0; i < MT_MAX_STANDART_FIBERS_COUNT; i++)
		{
			FiberContext& context = standartFiberContexts[i];
			context.fiber.Create(MT_STANDART_FIBER_STACK_SIZE, FiberMain, &context);
			standartFibersAvailable.Push( &context );
		}

		// create fiber pool (fibers with extended stack size)
		for (uint32 i = 0; i < MT_MAX_EXTENDED_FIBERS_COUNT; i++)
		{
			FiberContext& context = extendedFiberContexts[i];
			context.fiber.Create(MT_EXTENDED_FIBER_STACK_SIZE, FiberMain, &context);
			extendedFibersAvailable.Push( &context );
		}


		for (int16 i = 0; i < TaskGroup::MT_MAX_GROUPS_COUNT; i++)
		{
			if (i != TaskGroup::DEFAULT)
			{
				availableGroups.Push( TaskGroup(i) );
			}
		}

		groupStats[TaskGroup::DEFAULT].SetDebugIsFree(false);

		// create worker thread pool
		int32 totalThreadsCount = GetWorkersCount();
		for (int32 i = 0; i < totalThreadsCount; i++)
		{
			threadContext[i].SetThreadIndex(i);
			threadContext[i].taskScheduler = this;
			threadContext[i].thread.Start( MT_SCHEDULER_STACK_SIZE, ThreadMain, &threadContext[i] );
		}
	}

	TaskScheduler::~TaskScheduler()
	{
		int32 totalThreadsCount = GetWorkersCount();
		for (int32 i = 0; i < totalThreadsCount; i++)
		{
			threadContext[i].state.Store(internal::ThreadState::EXIT);
			threadContext[i].hasNewTasksEvent.Signal();
		}

		for (int32 i = 0; i < totalThreadsCount; i++)
		{
			threadContext[i].thread.Stop();
		}
	}

	ConcurrentQueueLIFO<FiberContext*>* TaskScheduler::GetFibersStorage(MT::StackRequirements::Type stackRequirements)
	{
		ConcurrentQueueLIFO<FiberContext*>* availableFibers = nullptr;
		switch(stackRequirements)
		{
		case MT::StackRequirements::STANDARD:
			availableFibers = &standartFibersAvailable;
			break;
		case MT::StackRequirements::EXTENDED:
			availableFibers = &extendedFibersAvailable;
			break;
		default:
			MT_REPORT_ASSERT("Unknown stack requrements");
		}

		return availableFibers;
	}

	FiberContext* TaskScheduler::RequestFiberContext(internal::GroupedTask& task)
	{
		FiberContext *fiberContext = task.awaitingFiber;
		if (fiberContext)
		{
			task.awaitingFiber = nullptr;
			return fiberContext;
		}


		MT::StackRequirements::Type stackRequirements = task.desc.stackRequirements;

		ConcurrentQueueLIFO<FiberContext*>* availableFibers = GetFibersStorage(stackRequirements);
		MT_VERIFY(availableFibers != nullptr, "Can't find fiber storage", return nullptr;);

		if (!availableFibers->TryPopBack(fiberContext))
		{
			MT_REPORT_ASSERT("Fibers pool is empty. Too many fibers running simultaneously.");
		}

		fiberContext->currentTask = task.desc;
		fiberContext->currentGroup = task.group;
		fiberContext->parentFiber = task.parentFiber;
		fiberContext->stackRequirements = stackRequirements;
		return fiberContext;
	}

	void TaskScheduler::ReleaseFiberContext(FiberContext* fiberContext)
	{
		MT_ASSERT(fiberContext != nullptr, "Can't release nullptr Fiber");

		MT::StackRequirements::Type stackRequirements = fiberContext->stackRequirements;
		fiberContext->Reset();

		ConcurrentQueueLIFO<FiberContext*>* availableFibers = GetFibersStorage(stackRequirements);
		MT_VERIFY(availableFibers != nullptr, "Can't find fiber storage", return;);

		availableFibers->Push(fiberContext);
	}

	FiberContext* TaskScheduler::ExecuteTask(internal::ThreadContext& threadContext, FiberContext* fiberContext)
	{
		MT_ASSERT(threadContext.thread.IsCurrentThread(), "Thread context sanity check failed");

		MT_ASSERT(fiberContext, "Invalid fiber context");
		MT_ASSERT(fiberContext->currentTask.IsValid(), "Invalid task");

		// Set actual thread context to fiber
		fiberContext->SetThreadContext(&threadContext);

		// Update task status
		fiberContext->SetStatus(FiberTaskStatus::RUNNED);

		MT_ASSERT(fiberContext->GetThreadContext()->thread.IsCurrentThread(), "Thread context sanity check failed");

		const void* poolUserData = fiberContext->currentTask.userData;
		TPoolTaskDestroy poolDestroyFunc = fiberContext->currentTask.poolDestroyFunc;

		// Run current task code
		Fiber::SwitchTo(threadContext.schedulerFiber, fiberContext->fiber);

		// If task was done
		FiberTaskStatus::Type taskStatus = fiberContext->GetStatus();
		if (taskStatus == FiberTaskStatus::FINISHED)
		{
			//destroy task (call dtor) for "fire and forget" type of task from TaskPool
			if (poolDestroyFunc != nullptr)
			{
				poolDestroyFunc(poolUserData);
			}

			TaskGroup taskGroup = fiberContext->currentGroup;

			TaskScheduler::TaskGroupDescription  & groupDesc = threadContext.taskScheduler->GetGroupDesc(taskGroup);

			// Update group status
			int groupTaskCount = groupDesc.Dec();
			MT_ASSERT(groupTaskCount >= 0, "Sanity check failed!");
			if (groupTaskCount == 0)
			{
				// Restore awaiting tasks
				threadContext.RestoreAwaitingTasks(taskGroup);

				// All restored tasks can be already finished on this line.
				// That's why you can't release groups from worker threads, if worker thread release group, than you can't Signal to released group.

				// Signal pending threads that group work is finished. Group can be destroyed after this call.
				groupDesc.Signal();

				fiberContext->currentGroup = TaskGroup::INVALID;
			}

			// Update total task count
			int allGroupTaskCount = threadContext.taskScheduler->allGroups.Dec();
			MT_ASSERT(allGroupTaskCount >= 0, "Sanity check failed!");
			if (allGroupTaskCount == 0)
			{
				// Notify all tasks in all group finished
				threadContext.taskScheduler->allGroups.Signal();
			}

			FiberContext* parentFiberContext = fiberContext->parentFiber;
			if (parentFiberContext != nullptr)
			{
				int childrenFibersCount = parentFiberContext->childrenFibersCount.DecFetch();
				MT_ASSERT(childrenFibersCount >= 0, "Sanity check failed!");

				if (childrenFibersCount == 0)
				{
					// This is a last subtask. Restore parent task
					MT_ASSERT(threadContext.thread.IsCurrentThread(), "Thread context sanity check failed");
					MT_ASSERT(parentFiberContext->GetThreadContext() == nullptr, "Inactive parent should not have a valid thread context");

					// WARNING!! Thread context can changed here! Set actual current thread context.
					parentFiberContext->SetThreadContext(&threadContext);

					MT_ASSERT(parentFiberContext->GetThreadContext()->thread.IsCurrentThread(), "Thread context sanity check failed");

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

		MT_ASSERT(taskStatus != FiberTaskStatus::RUNNED, "Incorrect task status")
		return nullptr;
	}


	void TaskScheduler::FiberMain(void* userData)
	{
		FiberContext& fiberContext = *(FiberContext*)(userData);
		for(;;)
		{
			MT_ASSERT(fiberContext.currentTask.IsValid(), "Invalid task in fiber context");
			MT_ASSERT(fiberContext.GetThreadContext(), "Invalid thread context");
			MT_ASSERT(fiberContext.GetThreadContext()->thread.IsCurrentThread(), "Thread context sanity check failed");

			fiberContext.currentTask.taskFunc( fiberContext, fiberContext.currentTask.userData );

			fiberContext.SetStatus(FiberTaskStatus::FINISHED);

#ifdef MT_INSTRUMENTED_BUILD
			fiberContext.GetThreadContext()->NotifyTaskFinished(fiberContext.currentTask);
#endif

			Fiber::SwitchTo(fiberContext.fiber, fiberContext.GetThreadContext()->schedulerFiber);
		}

	}


	bool TaskScheduler::TryStealTask(internal::ThreadContext& threadContext, internal::GroupedTask & task, uint32 workersCount)
	{
		if (workersCount <= 1)
		{
			return false;
		}

		uint32 victimIndex = threadContext.random.Get();

		for (uint32 attempt = 0; attempt < workersCount; attempt++)
		{
			uint32 index = victimIndex % workersCount;
			if (index == threadContext.workerIndex)
			{
				victimIndex++;
				index = victimIndex % workersCount;
			}

			internal::ThreadContext& victimContext = threadContext.taskScheduler->threadContext[index];
			if (victimContext.queue.TryPopFront(task))
			{
				return true;
			}

			victimIndex++;
		}
		return false;
	}


	void TaskScheduler::ThreadMain( void* userData )
	{
		internal::ThreadContext& context = *(internal::ThreadContext*)(userData);
		MT_ASSERT(context.taskScheduler, "Task scheduler must be not null!");
		context.schedulerFiber.CreateFromCurrentThreadAndRun(context.thread, ShedulerFiberMain, userData);
	}


	void TaskScheduler::ShedulerFiberMain( void* userData )
	{
		internal::ThreadContext& context = *(internal::ThreadContext*)(userData);
		MT_ASSERT(context.taskScheduler, "Task scheduler must be not null!");

#ifdef MT_INSTRUMENTED_BUILD
		context.NotifyThreadCreate(context.workerIndex);
#endif


		uint32 workersCount = context.taskScheduler->GetWorkersCount();

		int32 totalThreadsCount = context.taskScheduler->threadsCount.LoadRelaxed();

		context.taskScheduler->startedThreadsCount.IncFetch();

		//Simple spinlock until all threads is started and initialized
		for(;;)
		{
			int32 initializedThreadsCount = context.taskScheduler->startedThreadsCount.Load();
			if (initializedThreadsCount == totalThreadsCount)
			{
				break;
			}
			Thread::Sleep(1);
		}


		HardwareFullMemoryBarrier();

#ifdef MT_INSTRUMENTED_BUILD
		context.NotifyThreadStart(context.workerIndex);
#endif

		while(context.state.Load() != internal::ThreadState::EXIT)
		{
			internal::GroupedTask task;
			if (context.queue.TryPopBack(task) || TryStealTask(context, task, workersCount) )
			{
				// There is a new task
				FiberContext* fiberContext = context.taskScheduler->RequestFiberContext(task);
				MT_ASSERT(fiberContext, "Can't get execution context from pool");
				MT_ASSERT(fiberContext->currentTask.IsValid(), "Sanity check failed");
				MT_ASSERT(fiberContext->stackRequirements == task.desc.stackRequirements, "Sanity check failed");

				while(fiberContext)
				{
#ifdef MT_INSTRUMENTED_BUILD
					context.NotifyTaskResumed(fiberContext->currentTask);
#endif
					// prevent invalid fiber resume from child tasks, before ExecuteTask is done
					fiberContext->childrenFibersCount.IncFetch();

					FiberContext* parentFiber = ExecuteTask(context, fiberContext);

					FiberTaskStatus::Type taskStatus = fiberContext->GetStatus();

					//release guard
					int childrenFibersCount = fiberContext->childrenFibersCount.DecFetch();

					// Can drop fiber context - task is finished
					if (taskStatus == FiberTaskStatus::FINISHED)
					{
						MT_ASSERT( childrenFibersCount == 0, "Sanity check failed");
						context.taskScheduler->ReleaseFiberContext(fiberContext);

						// If parent fiber is exist transfer flow control to parent fiber, if parent fiber is null, exit
						fiberContext = parentFiber;
					} else
					{
						MT_ASSERT( childrenFibersCount >= 0, "Sanity check failed");

						// No subtasks here and status is not finished, this mean all subtasks already finished before parent return from ExecuteTask
						if (childrenFibersCount == 0)
						{
							MT_ASSERT(parentFiber == nullptr, "Sanity check failed");
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
#ifdef MT_INSTRUMENTED_BUILD
				context.NotifyThreadIdleBegin(context.workerIndex);
#endif

				// Queue is empty and stealing attempt failed
				// Wait new events
				context.hasNewTasksEvent.Wait(2000);

#ifdef MT_INSTRUMENTED_BUILD
				context.NotifyThreadIdleEnd(context.workerIndex);
#endif

			}

		} // main thread loop

#ifdef MT_INSTRUMENTED_BUILD
		context.NotifyThreadStop(context.workerIndex);
#endif

	}

	void TaskScheduler::RunTasksImpl(ArrayView<internal::TaskBucket>& buckets, FiberContext * parentFiber, bool restoredFromAwaitState)
	{
		// This storage is necessary to calculate how many tasks we add to different groups
		int newTaskCountInGroup[TaskGroup::MT_MAX_GROUPS_COUNT];

		// Default value is 0
		memset(&newTaskCountInGroup[0], 0, sizeof(newTaskCountInGroup));

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

				task.parentFiber = parentFiber;

				int idx = task.group.GetValidIndex();
				MT_ASSERT(idx >= 0 && idx < TaskGroup::MT_MAX_GROUPS_COUNT, "Invalid index");
				newTaskCountInGroup[idx]++;
			}

			count += bucket.count;
		}

		// Increments child fibers count on parent fiber
		if (parentFiber)
		{
			parentFiber->childrenFibersCount.AddFetch((int)count);
		}

		if (restoredFromAwaitState == false)
		{
			// Increase the number of active tasks in the group using data from temporary storage
			for (size_t i = 0; i < TaskGroup::MT_MAX_GROUPS_COUNT; i++)
			{
				int groupNewTaskCount = newTaskCountInGroup[i];
				if (groupNewTaskCount > 0)
				{
					groupStats[i].Reset();
					groupStats[i].Add((uint32)groupNewTaskCount);
				}
			}

			// Increments all task in progress counter
			allGroups.Reset();
			allGroups.Add((uint32)count);
		} else
		{
			// If task's restored from await state, counters already in correct state
		}

		// Add to thread queue
		for (size_t i = 0; i < buckets.Size(); ++i)
		{
			int bucketIndex = roundRobinThreadIndex.IncFetch() % threadsCount.LoadRelaxed();
			internal::ThreadContext & context = threadContext[bucketIndex];

			internal::TaskBucket& bucket = buckets[i];

			context.queue.PushRange(bucket.tasks, bucket.count);
			context.hasNewTasksEvent.Signal();
		}
	}

	void TaskScheduler::RunAsync(TaskGroup group, const TaskHandle* taskHandleArray, uint32 taskHandleCount)
	{
		MT_ASSERT(!IsWorkerThread(), "Can't use RunAsync inside Task. Use FiberContext.RunAsync() instead.");

		ArrayView<internal::GroupedTask> buffer(MT_ALLOCATE_ON_STACK(sizeof(internal::GroupedTask) * taskHandleCount), taskHandleCount);

		uint32 bucketCount = MT::Min((uint32)GetWorkersCount(), taskHandleCount);
		ArrayView<internal::TaskBucket> buckets(MT_ALLOCATE_ON_STACK(sizeof(internal::TaskBucket) * bucketCount), bucketCount);

		internal::DistibuteDescriptions(group, taskHandleArray, buffer, buckets);
		RunTasksImpl(buckets, nullptr, false);
	}

	bool TaskScheduler::WaitGroup(TaskGroup group, uint32 milliseconds)
	{
		MT_VERIFY(IsWorkerThread() == false, "Can't use WaitGroup inside Task. Use FiberContext.WaitGroupAndYield() instead.", return false);

		TaskScheduler::TaskGroupDescription  & groupDesc = GetGroupDesc(group);
		return groupDesc.Wait(milliseconds);
	}

	bool TaskScheduler::WaitAll(uint32 milliseconds)
	{
		MT_VERIFY(IsWorkerThread() == false, "Can't use WaitAll inside Task.", return false);

		return allGroups.Wait(milliseconds);
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

	int32 TaskScheduler::GetWorkersCount() const
	{
		return threadsCount.LoadRelaxed();
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

	TaskGroup TaskScheduler::CreateGroup()
	{
		MT_ASSERT(IsWorkerThread() == false, "Can't use CreateGroup inside Task.");

		TaskGroup group;
		if (!availableGroups.TryPopBack(group))
		{
			MT_REPORT_ASSERT("Group pool is empty");
		}

		int idx = group.GetValidIndex();

		MT_ASSERT(groupStats[idx].GetDebugIsFree() == true, "Bad logic!");
		groupStats[idx].SetDebugIsFree(false);

		return group;
	}

	void TaskScheduler::ReleaseGroup(TaskGroup group)
	{
		MT_ASSERT(IsWorkerThread() == false, "Can't use ReleaseGroup inside Task.");
		MT_ASSERT(group.IsValid(), "Invalid group ID");

		int idx = group.GetValidIndex();

		MT_ASSERT(groupStats[idx].GetDebugIsFree() == false, "Group already released");
		groupStats[idx].SetDebugIsFree(true);

		availableGroups.Push(group);
	}

	TaskScheduler::TaskGroupDescription & TaskScheduler::GetGroupDesc(TaskGroup group)
	{
		MT_ASSERT(group.IsValid(), "Invalid group ID");

		int idx = group.GetValidIndex();
		TaskScheduler::TaskGroupDescription & groupDesc = groupStats[idx];

		MT_ASSERT(groupDesc.GetDebugIsFree() == false, "Invalid group");
		return groupDesc;
	}
}


