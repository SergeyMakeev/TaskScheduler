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

#include "Tests.h"
#include <UnitTest++.h>
#include <MTScheduler.h>
#include <MTStaticVector.h>

SUITE(SimpleTests)
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SimpleTask
{
	MT_DECLARE_TASK(SimpleTask, MT::StackRequirements::STANDARD, MT::TaskPriority::NORMAL, MT::Color::Blue);

	static const int sourceData = 0xFF33FF;
	int resultData;

	SimpleTask() : resultData(0) {}

	void Do(MT::FiberContext&)
	{
		resultData = sourceData;
	}

	int GetSourceData()
	{
		return sourceData;
	}
};

// Checks one simple task
TEST(RunOneSimpleTask)
{
	MT::TaskScheduler scheduler;

	SimpleTask task;
	scheduler.RunAsync(MT::TaskGroup::Default(), &task, 1);

	CHECK(scheduler.WaitAll(1000));
	CHECK_EQUAL(task.GetSourceData(), task.resultData);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct ALotOfTasks
{
	MT_DECLARE_TASK(ALotOfTasks, MT::StackRequirements::STANDARD, MT::TaskPriority::NORMAL, MT::Color::Blue);

	MT::Atomic32<int32>* counter;

	void Do(MT::FiberContext&)
	{
		counter->IncFetch();
		MT::SpinSleepMilliSeconds(1);
	}
};

// Checks one simple task
TEST(ALotOfTasks)
{
	MT::TaskScheduler scheduler;

	MT::Atomic32<int32> counter;

	static const int TASK_COUNT = 1000;

	ALotOfTasks tasks[TASK_COUNT];

	for (size_t i = 0; i < MT_ARRAY_SIZE(tasks); ++i)
		tasks[i].counter = &counter;

	scheduler.RunAsync(MT::TaskGroup::Default(), &tasks[0], MT_ARRAY_SIZE(tasks));

	int timeout = (TASK_COUNT / scheduler.GetWorkersCount()) * 2000;

	CHECK(scheduler.WaitGroup(MT::TaskGroup::Default(), timeout));
	CHECK_EQUAL(TASK_COUNT, counter.Load());
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


struct WorkerThreadState
{
	uint32 counterPhase0;
	uint32 counterPhase1;

	WorkerThreadState()
	{
		Reset();
	}

	void Reset()
	{
		counterPhase0 = 0;
		counterPhase1 = 0;
	}
};


WorkerThreadState workerStates[64];

uint32 TASK_COUNT_PER_WORKER = 0;

MT::Atomic32<uint32> finishedTaskCount;

struct YieldTask
{
	MT::Atomic32<uint32> counter;

	MT_DECLARE_TASK(YieldTask, MT::StackRequirements::STANDARD, MT::TaskPriority::NORMAL, MT::Color::Blue);

	YieldTask()
	{
		counter.Store(0);
	}


	 volatile WorkerThreadState* GetWorkerState( volatile uint32 workerIndex) volatile
	{
		MT_ASSERT(workerIndex < MT_ARRAY_SIZE(workerStates), "Invalid worker index");
		volatile WorkerThreadState& state = workerStates[workerIndex];
		return &state;
	}

	void Do(MT::FiberContext& context)
	{
		volatile WorkerThreadState* state0 = GetWorkerState( context.GetThreadContext()->workerIndex );

		// phase 0
		CHECK_EQUAL((uint32)1, counter.IncFetch());
		state0->counterPhase0++;
		context.Yield();

		// worker index can be changed after yield, get actual index
		volatile WorkerThreadState* state1 = GetWorkerState( context.GetThreadContext()->workerIndex );

		//I check that all the tasks (on this worker) have passed phase0 before executing phase1
		CHECK_EQUAL(TASK_COUNT_PER_WORKER, state1->counterPhase0);

		// phase 1
		CHECK_EQUAL((uint32)2, counter.IncFetch());
		state1->counterPhase1++;

		finishedTaskCount.IncFetch();
	}
};


TEST(YieldTasks)
{
	// Disable task stealing (for testing purposes only)
#ifdef MT_INSTRUMENTED_BUILD
	MT::TaskScheduler scheduler(0, nullptr, nullptr, MT::TaskStealingMode::DISABLED);
#else
	MT::TaskScheduler scheduler(0, nullptr, MT::TaskStealingMode::DISABLED);
#endif

	finishedTaskCount.Store(0);

	int32 workersCount = scheduler.GetWorkersCount();
	TASK_COUNT_PER_WORKER = workersCount * 4;
	int32 taskCount = workersCount * TASK_COUNT_PER_WORKER;

	MT::HardwareFullMemoryBarrier();

	MT::StaticVector<YieldTask, 512> tasks;
	for(int32 i = 0; i < taskCount; i++)
	{
		tasks.PushBack(YieldTask());
	}

	for(int32 i = 0; i < workersCount; i++)
	{
		WorkerThreadState& state = workerStates[i];
		state.Reset();
	}


	scheduler.RunAsync(MT::TaskGroup::Default(), tasks.Begin(), (uint32)tasks.Size());

	CHECK(scheduler.WaitGroup(MT::TaskGroup::Default(), 10000));

	for(int32 i = 0; i < workersCount; i++)
	{
		WorkerThreadState& state = workerStates[i];

		CHECK_EQUAL(TASK_COUNT_PER_WORKER, state.counterPhase0);
		CHECK_EQUAL(TASK_COUNT_PER_WORKER, state.counterPhase1);
	}

	CHECK_EQUAL(taskCount, (int32)finishedTaskCount.Load());

	printf("Yield test: %d tasks finished, used %d workers\n", taskCount, workersCount);

}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
