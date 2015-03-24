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
	namespace internal
	{
		static const size_t TASK_BUFFER_CAPACITY = 4096;

		ThreadContext::ThreadContext()
			: lastActiveFiberContext(nullptr)
			, taskScheduler(nullptr)
			, hasNewTasksEvent(EventReset::AUTOMATIC, true)
			, state(ThreadState::ALIVE)
			, descBuffer(TASK_BUFFER_CAPACITY)
		{
		}

		ThreadContext::~ThreadContext()
		{
		}

		void ThreadContext::RestoreAwaitingTasks(TaskGroup::Type taskGroup)
		{
			ASSERT(taskScheduler, "Invalid Task Scheduler");
			ASSERT(taskScheduler->IsWorkerThread(), "Can't use RunAsync outside Task. Use TaskScheduler.RunAsync() instead.");

			ConcurrentQueueLIFO<FiberContext*> & groupQueue = taskScheduler->waitTaskQueues[taskGroup];

			if (groupQueue.IsEmpty())
			{
				return;
			}

			//copy awaiting tasks list to stack
			stack_array<FiberContext*, MT_MAX_FIBERS_COUNT> groupQueueCopy(MT_MAX_FIBERS_COUNT, nullptr);
			size_t taskCount = groupQueue.PopAll(groupQueueCopy.begin(), groupQueueCopy.size());

			fixed_array<internal::GroupedTask> buffer(&descBuffer.front(), taskCount);

			TaskScheduler & scheduler = *(taskScheduler);
			size_t bucketCount = Min((size_t)scheduler.GetWorkerCount(), taskCount);
			fixed_array<internal::TaskBucket>	buckets(ALLOCATE_ON_STACK(sizeof(internal::TaskBucket) * bucketCount), bucketCount);

			internal::DistibuteDescriptions(TaskGroup::GROUP_UNDEFINED, groupQueueCopy.begin(), buffer, buckets);
			scheduler.RunTasksImpl(buckets, nullptr, true);
		}

	}

}
