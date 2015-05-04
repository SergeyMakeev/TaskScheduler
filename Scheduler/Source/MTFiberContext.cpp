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

	void FiberContext::SetThreadContext(internal::ThreadContext * _threadContext)
	{
		if (_threadContext)
		{
			_threadContext->lastActiveFiberContext = this;
		}

		threadContext = _threadContext;
	}

	internal::ThreadContext* FiberContext::GetThreadContext()
	{
		return threadContext;
	}

	void FiberContext::Reset()
	{
		ASSERT(childrenFibersCount.Get() == 0, "Can't release fiber with active children fibers");

		currentGroup = TaskGroup::GROUP_UNDEFINED;
		currentTask = internal::TaskDesc();
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

#ifdef MT_INSTRUMENTED_BUILD
		threadContext->NotifyTaskYielded(currentTask);
#endif

		// Yielding, so reset thread context
		threadContext = nullptr;

		//switch to scheduler
		Fiber::SwitchTo(fiber, schedulerFiber);
	}

	void FiberContext::RunSubtasksAndYieldImpl(WrapperArray<internal::TaskBucket>& buckets)
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

#ifdef MT_INSTRUMENTED_BUILD
		threadContext->NotifyTaskYielded(currentTask);
#endif

		// Yielding, so reset thread context
		threadContext = nullptr;

		//switch to scheduler
		Fiber::SwitchTo(fiber, schedulerFiber);
	}


}
