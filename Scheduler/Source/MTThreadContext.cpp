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
		// Prime numbers for linear congruential generator seed
		static const uint32 primeNumbers[] = {
			128473, 135349, 159499, 173839, 209213, 241603, 292709, 314723,
			343943, 389299, 419473, 465169, 518327, 649921, 748271, 851087,
			862171, 974551, 1002973, 1034639, 1096289, 1153123, 1251037, 1299269,
			1272941, 1252151, 1231091, 1206761, 1185469, 1169933, 1141351, 1011583 };

		uint32 GetPrimeNumber(uint32 index)
		{
			return primeNumbers[index % MT_ARRAY_SIZE(primeNumbers)];
		}



		ThreadContext::ThreadContext()
			: lastActiveFiberContext(nullptr)
			, taskScheduler(nullptr)
			, hasNewTasksEvent(EventReset::AUTOMATIC, true)
			, state(ThreadState::ALIVE)
			, workerIndex(0)
		{
			 descBuffer = Memory::Alloc( sizeof(internal::GroupedTask) * TASK_BUFFER_CAPACITY );
		}

		ThreadContext::~ThreadContext()
		{
			Memory::Free(descBuffer);
			descBuffer = nullptr;
		}

		void ThreadContext::SetThreadIndex(uint32 threadIndex)
		{
			workerIndex = threadIndex;
			random.SetSeed( GetPrimeNumber(threadIndex) );
		}

		void ThreadContext::RestoreAwaitingTasks(TaskGroup taskGroup)
		{
			MT_ASSERT(taskScheduler, "Invalid Task Scheduler");
			MT_ASSERT(taskScheduler->IsWorkerThread(), "Can't use RunAsync outside Task. Use TaskScheduler.RunAsync() instead.");

			TaskScheduler::TaskGroupDescription  & groupDesc = taskScheduler->GetGroupDesc(taskGroup);
			ConcurrentQueueLIFO<FiberContext*> & groupQueue = groupDesc.GetWaitQueue();

			if (groupQueue.IsEmpty())
			{
				return;
			}

			//copy awaiting tasks list to stack
			const int maximumFibersCount = MT_MAX_STANDART_FIBERS_COUNT + MT_MAX_EXTENDED_FIBERS_COUNT;
			StackArray<FiberContext*, maximumFibersCount> groupQueueCopy(maximumFibersCount, nullptr);
			size_t taskCount = groupQueue.PopAll(groupQueueCopy.Begin(), groupQueueCopy.Size());

			ArrayView<internal::GroupedTask> buffer(descBuffer, taskCount);

			TaskScheduler & scheduler = *(taskScheduler);
			size_t bucketCount = MT::Min((size_t)scheduler.GetWorkersCount(), taskCount);
			ArrayView<internal::TaskBucket>	buckets(MT_ALLOCATE_ON_STACK(sizeof(internal::TaskBucket) * bucketCount), bucketCount);

			internal::DistibuteDescriptions( TaskGroup(TaskGroup::ASSIGN_FROM_CONTEXT), groupQueueCopy.Begin(), buffer, buckets);
			scheduler.RunTasksImpl(buckets, nullptr, true);
		}

#ifdef MT_INSTRUMENTED_BUILD

		void ThreadContext::NotifyTaskFinished(const internal::TaskDesc & desc)
		{
			if (IProfilerEventListener* eventListener = taskScheduler->GetProfilerEventListener())
			{
				eventListener->OnTaskFinished(desc.debugColor, desc.debugID);
			}
		}

		void ThreadContext::NotifyTaskResumed(const internal::TaskDesc & desc)
		{
			if (IProfilerEventListener* eventListener = taskScheduler->GetProfilerEventListener())
			{
				eventListener->OnTaskResumed(desc.debugColor, desc.debugID);
			}

		}

		void ThreadContext::NotifyTaskYielded(const internal::TaskDesc & desc)
		{
			if (IProfilerEventListener* eventListener = taskScheduler->GetProfilerEventListener())
			{
				eventListener->OnTaskYielded(desc.debugColor, desc.debugID);
			}
		}

		void ThreadContext::NotifyThreadCreate(uint32 threadIndex)
		{
			if (IProfilerEventListener* eventListener = taskScheduler->GetProfilerEventListener())
			{
				eventListener->OnThreadCreated(threadIndex);
			}
		}

		void ThreadContext::NotifyThreadStart(uint32 threadIndex)
		{
			if (IProfilerEventListener* eventListener = taskScheduler->GetProfilerEventListener())
			{
				eventListener->OnThreadStarted(threadIndex);
			}
		}

		void ThreadContext::NotifyThreadStop(uint32 threadIndex)
		{
			if (IProfilerEventListener* eventListener = taskScheduler->GetProfilerEventListener())
			{
				eventListener->OnThreadStoped(threadIndex);
			}
		}


		void ThreadContext::NotifyThreadIdleBegin(uint32 threadIndex)
		{
			if (IProfilerEventListener* eventListener = taskScheduler->GetProfilerEventListener())
			{
				eventListener->OnThreadIdleBegin(threadIndex);
			}
		}

		void ThreadContext::NotifyThreadIdleEnd(uint32 threadIndex)
		{
			if (IProfilerEventListener* eventListener = taskScheduler->GetProfilerEventListener())
			{
				eventListener->OnThreadIdleEnd(threadIndex);
			}
		}

#endif

	}

}
