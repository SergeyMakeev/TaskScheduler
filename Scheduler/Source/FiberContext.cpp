#include <MTScheduler.h>

namespace MT
{
	FiberContext::FiberContext()
		: threadContext(nullptr)
		, taskStatus(FiberTaskStatus::UNKNOWN)
		, currentGroup(TaskGroup::GROUP_UNDEFINED)
		, childrenFibersCount(0)
		, parentFiber(nullptr)
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

	void FiberContext::RunSubtasksAndYieldImpl(fixed_array<TaskBucket>& buckets)
	{
		ASSERT(threadContext, "Sanity check failed!");
		ASSERT(threadContext->taskScheduler->IsWorkerThread(), "Can't use RunSubtasksAndYield outside Task. Use TaskScheduler.WaitGroup() instead.");
		ASSERT(threadContext->thread.IsCurrentThread(), "Thread context sanity check failed");

		// add to scheduler
		threadContext->taskScheduler->RunTasksImpl(buckets, this, false);

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


}
