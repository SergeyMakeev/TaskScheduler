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

		// Prime numbers for linear congruential generator seed
		static const uint32 primeNumbers[] = {
			128473, 135349, 159499, 173839, 209213, 241603, 292709, 314723,
			343943, 389299, 419473, 465169, 518327, 649921, 748271, 851087,
			862171, 974551, 1002973, 1034639, 1096289, 1153123, 1251037, 1299269,
			1272941, 1252151, 1231091, 1206761, 1185469, 1169933, 1141351, 1011583 };

		uint32 GetPrimeNumber(uint32 index)
		{
			return primeNumbers[index % ARRAY_SIZE(primeNumbers)];
		}



		ThreadContext::ThreadContext()
			: lastActiveFiberContext(nullptr)
			, taskScheduler(nullptr)
			, hasNewTasksEvent(EventReset::AUTOMATIC, true)
			, state(ThreadState::ALIVE)
			, descBuffer(TASK_BUFFER_CAPACITY)
			, workerIndex(0)
		{
		}

		ThreadContext::~ThreadContext()
		{
		}

		void ThreadContext::SetThreadIndex(uint32 threadIndex)
		{
			workerIndex = threadIndex;
			random.SetSeed( GetPrimeNumber(threadIndex) );
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
			StackArray<FiberContext*, MT_MAX_FIBERS_COUNT> groupQueueCopy(MT_MAX_FIBERS_COUNT, nullptr);
			size_t taskCount = groupQueue.PopAll(groupQueueCopy.Begin(), groupQueueCopy.Size());

			WrapperArray<internal::GroupedTask> buffer(&descBuffer.front(), taskCount);

			TaskScheduler & scheduler = *(taskScheduler);
			size_t bucketCount = Min((size_t)scheduler.GetWorkerCount(), taskCount);
			WrapperArray<internal::TaskBucket>	buckets(ALLOCATE_ON_STACK(sizeof(internal::TaskBucket) * bucketCount), bucketCount);

			internal::DistibuteDescriptions(TaskGroup::GROUP_UNDEFINED, groupQueueCopy.Begin(), buffer, buckets);
			scheduler.RunTasksImpl(buckets, nullptr, true);
		}

#ifdef MT_INSTRUMENTED_BUILD

		void ThreadContext::NotifyTaskFinished(const internal::TaskDesc & desc)
		{
			desc;

			ProfileEventDesc eventDesc;
			eventDesc.type = ProfileEventType::TASK_DONE;
			eventDesc.timeStampMicroSeconds = MT::GetTimeMicroSeconds();
			profileEvents.Push(std::move(eventDesc));
		}

		void ThreadContext::NotifyTaskResumed(const internal::TaskDesc & desc)
		{
			desc;

			ProfileEventDesc eventDesc;
			eventDesc.type = ProfileEventType::TASK_RESUME;
			eventDesc.timeStampMicroSeconds = MT::GetTimeMicroSeconds();
			profileEvents.Push(std::move(eventDesc));
		}

		void ThreadContext::NotifyTaskYielded(const internal::TaskDesc & desc)
		{
			desc;

			ProfileEventDesc eventDesc;
			eventDesc.type = ProfileEventType::TASK_YIELD;
			eventDesc.timeStampMicroSeconds = MT::GetTimeMicroSeconds();
			profileEvents.Push(std::move(eventDesc));
		}


#endif

	}

}
