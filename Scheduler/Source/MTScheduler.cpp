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

	TaskScheduler::TaskScheduler(uint32 workerThreadsCount)
		: roundRobinThreadIndex(0)
		, startedThreadsCount(0)
	{

#ifdef MT_INSTRUMENTED_BUILD
		webServerPort = profilerWebServer.Serve(8080, 8090);
		//initialize start time
		startTime = MT::GetTimeMicroSeconds();
#endif

		if (workerThreadsCount != 0)
		{
			threadsCount = MT::Clamp(workerThreadsCount, (uint32)1, (uint32)MT_MAX_THREAD_COUNT);
		} else
		{
			//query number of processor
			threadsCount = (uint32)MT::Clamp(Thread::GetNumberOfHardwareThreads() - 2, 1, (int)MT_MAX_THREAD_COUNT);
		}

		// create fiber pool
		for (uint32 i = 0; i < MT_MAX_FIBERS_COUNT; i++)
		{
			FiberContext& context = fiberContext[i];
			context.fiber.Create(MT_FIBER_STACK_SIZE, FiberMain, &context);
			availableFibers.Push( &context );
		}

		for (uint32 i = 0; i < TaskGroup::MT_MAX_GROUPS_COUNT; i++)
		{
			if (i != TaskGroup::DEFAULT)
			{
				availableGroups.Push( TaskGroup(i) );
			}
		}

		groupStats[TaskGroup::DEFAULT].debugIsFree = false;

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

		if (!availableFibers.TryPopBack(fiberContext))
		{
			MT_ASSERT(false, "Fibers pool is empty");
		}

		fiberContext->currentTask = task.desc;
		fiberContext->currentGroup = task.group;
		fiberContext->parentFiber = task.parentFiber;
		return fiberContext;
	}

	void TaskScheduler::ReleaseFiberContext(FiberContext* fiberContext)
	{
		MT_ASSERT(fiberContext != nullptr, "Can't release nullptr Fiber");
		fiberContext->Reset();
		availableFibers.Push(fiberContext);
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

		// Run current task code
		Fiber::SwitchTo(threadContext.schedulerFiber, fiberContext->fiber);

		// If task was done
		FiberTaskStatus::Type taskStatus = fiberContext->GetStatus();
		if (taskStatus == FiberTaskStatus::FINISHED)
		{
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
				int childrenFibersCount = parentFiberContext->childrenFibersCount.Dec();
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
		context.schedulerFiber.CreateFromThread(context.thread);

		uint32 workersCount = context.taskScheduler->GetWorkerCount();

		context.taskScheduler->startedThreadsCount.Inc();

		//Spinlock until all threads started and initialized
		while(true)
		{
			if (context.taskScheduler->startedThreadsCount.Get() == (int)context.taskScheduler->threadsCount)
			{
				break;
			}
			Thread::Sleep(1);
		}

		while(context.state.Get() != internal::ThreadState::EXIT)
		{
			internal::GroupedTask task;
			if (context.queue.TryPopBack(task) || TryStealTask(context, task, workersCount) )
			{
				// There is a new task
				FiberContext* fiberContext = context.taskScheduler->RequestFiberContext(task);
				MT_ASSERT(fiberContext, "Can't get execution context from pool");
				MT_ASSERT(fiberContext->currentTask.IsValid(), "Sanity check failed");

				while(fiberContext)
				{
#ifdef MT_INSTRUMENTED_BUILD
					context.NotifyTaskResumed(fiberContext->currentTask);
#endif

					// prevent invalid fiber resume from child tasks, before ExecuteTask is done
					fiberContext->childrenFibersCount.Inc();

					FiberContext* parentFiber = ExecuteTask(context, fiberContext);

					FiberTaskStatus::Type taskStatus = fiberContext->GetStatus();

					//release guard
					int childrenFibersCount = fiberContext->childrenFibersCount.Dec();

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
				int64 waitFrom = MT::GetTimeMicroSeconds();
#endif

				// Queue is empty and stealing attempt failed
				// Wait new events
				context.hasNewTasksEvent.Wait(2000);

#ifdef MT_INSTRUMENTED_BUILD
				int64 waitTo = MT::GetTimeMicroSeconds();
				context.NotifyWorkerAwait(waitFrom, waitTo);
#endif

			}

		} // main thread loop
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
			parentFiber->childrenFibersCount.Add((uint32)count);
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
			int bucketIndex = roundRobinThreadIndex.Inc() % threadsCount;
			internal::ThreadContext & context = threadContext[bucketIndex];

			internal::TaskBucket& bucket = buckets[i];

			context.queue.PushRange(bucket.tasks, bucket.count);
			context.hasNewTasksEvent.Signal();
		}
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

	TaskGroup TaskScheduler::CreateGroup()
	{
		MT_ASSERT(IsWorkerThread() == false, "Can't use CreateGroup inside Task.");

		TaskGroup group;
		if (!availableGroups.TryPopBack(group))
		{
			MT_ASSERT(false, "Group pool is empty");
		}

		int idx = group.GetValidIndex();

		MT_ASSERT(groupStats[idx].debugIsFree == true, "Bad logic!");
		groupStats[idx].debugIsFree = false;

		return group;
	}

	void TaskScheduler::ReleaseGroup(TaskGroup group)
	{
		MT_ASSERT(IsWorkerThread() == false, "Can't use ReleaseGroup inside Task.");
		MT_ASSERT(group.IsValid(), "Invalid group ID");

		int idx = group.GetValidIndex();

		MT_ASSERT(groupStats[idx].debugIsFree == false, "Group already released");
		groupStats[idx].debugIsFree = true;

		availableGroups.Push(group);
	}

	TaskScheduler::TaskGroupDescription & TaskScheduler::GetGroupDesc(TaskGroup group)
	{
		MT_ASSERT(group.IsValid(), "Invalid group ID");

		int idx = group.GetValidIndex();
		TaskScheduler::TaskGroupDescription & groupDesc = groupStats[idx];

		MT_ASSERT(groupDesc.debugIsFree == false, "Invalid group");
		return groupDesc;
	}


#ifdef MT_INSTRUMENTED_BUILD

	size_t TaskScheduler::GetProfilerEvents(uint32 workerIndex, ProfileEventDesc * dstBuffer, size_t dstBufferSize)
	{
		if (workerIndex >= MT_MAX_THREAD_COUNT)
		{
			return 0;
		}

		size_t elementsCount = threadContext[workerIndex].profileEvents.PopAll(dstBuffer, dstBufferSize);
		return elementsCount;
	}

	void TaskScheduler::UpdateProfiler()
	{
		profilerWebServer.Update(*this);
	}

	int32 TaskScheduler::GetWebServerPort() const
	{
		return webServerPort;
	}



#endif
}
