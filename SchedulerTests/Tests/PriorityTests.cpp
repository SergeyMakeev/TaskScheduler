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

/*

Check that every worker thread executed tasks in specified priority. HIGH/NORMAL/LOW

*/
SUITE(PriorityTests)
{
	static const uint32 TASK_COUNT = 512;

	MT::Atomic32<int32> switchCountToNormal;
	MT::Atomic32<int32> switchCountToLow;


	struct ThreadState
	{
		uint32 taskPrio;
		uint32 highProcessed;
		uint32 normalProcessed;
		uint32 lowProcessed;
		byte cacheLine[64];

		ThreadState()
		{
			Reset();
		}

		void Reset()
		{
			taskPrio = 0;
			highProcessed = 0;
			normalProcessed = 0;
			lowProcessed = 0;
		}
	};

	ThreadState workerState[64];

	struct TaskHigh
	{
		MT_DECLARE_TASK(TaskHigh, MT::StackRequirements::STANDARD, MT::TaskPriority::HIGH, MT::Color::Blue);

		uint32 id;

		TaskHigh(uint32 _id)
			: id(_id)
		{
		}

		void Do(MT::FiberContext& ctx)
		{
			uint32 workerIndex = ctx.GetThreadContext()->workerIndex;
			MT_ASSERT(workerIndex < MT_ARRAY_SIZE(workerState), "Invalid worker index");
			ThreadState& state = workerState[workerIndex];

			CHECK_EQUAL((uint32)0, state.normalProcessed);
			CHECK_EQUAL((uint32)0, state.lowProcessed);

			state.highProcessed++;

			// Check we in right state (executing HIGH priority tasks)
			CHECK_EQUAL((uint32)0, state.taskPrio);

		}
	};


	struct TaskNormal
	{
		MT_DECLARE_TASK(TaskNormal, MT::StackRequirements::STANDARD, MT::TaskPriority::NORMAL, MT::Color::Blue);

		uint32 id;

		TaskNormal(uint32 _id)
			: id(_id)
		{
		}

		void Do(MT::FiberContext& ctx)
		{
			uint32 workerIndex = ctx.GetThreadContext()->workerIndex;
			MT_ASSERT(workerIndex < MT_ARRAY_SIZE(workerState), "Invalid worker index");
			ThreadState& state = workerState[workerIndex];

			CHECK_EQUAL((uint32)0, state.lowProcessed);

			state.normalProcessed++;

			//if state is set to HIGH tasks, change state to NORMAL tasks
			if (state.taskPrio == 0)
			{
				state.taskPrio = 1;
				switchCountToNormal.IncFetch();
			}

			// Check we in right state (executing NORMAL priority tasks)
			CHECK_EQUAL((uint32)1, state.taskPrio);
		}
	};

	struct TaskLow
	{
		MT_DECLARE_TASK(TaskLow, MT::StackRequirements::STANDARD, MT::TaskPriority::LOW, MT::Color::Blue);

		uint32 id;

		TaskLow(uint32 _id)
			: id(_id)
		{
		}


		void Do(MT::FiberContext& ctx)
		{
			uint32 workerIndex = ctx.GetThreadContext()->workerIndex;
			MT_ASSERT(workerIndex < MT_ARRAY_SIZE(workerState), "Invalid worker index");
			ThreadState& state = workerState[workerIndex];

			state.lowProcessed++;

			//if state is set to NORMAL tasks, change state to LOW tasks
			if (state.taskPrio == 1)
			{
				state.taskPrio = 2;
				switchCountToLow.IncFetch();
			}

			// Check we in right state (executing LOW priority tasks)
			CHECK_EQUAL((uint32)2, state.taskPrio);
		}
	};




	TEST(SimplePriorityTest)
	{
		MT::TaskPool<TaskLow, TASK_COUNT> lowPriorityTasksPool;
		MT::TaskPool<TaskNormal, TASK_COUNT> normalPriorityTasksPool;
		MT::TaskPool<TaskHigh, TASK_COUNT> highPriorityTasksPool;

		// Disable task stealing (for testing purposes only)
#ifdef MT_INSTRUMENTED_BUILD
		MT::TaskScheduler scheduler(0, nullptr, nullptr, MT::TaskStealingMode::DISABLED);
#else
		MT::TaskScheduler scheduler(0, nullptr, MT::TaskStealingMode::DISABLED);
#endif

		// Use task handles to add multiple tasks with different priorities in one RunAsync call
		MT::TaskHandle taskHandles[TASK_COUNT*3];

		uint32 index = 0;
		for(uint32 i = 0; i < TASK_COUNT; i++)
		{
			taskHandles[index] = lowPriorityTasksPool.Alloc(TaskLow(i));
			index++;
		}

		for(uint32 i = 0; i < TASK_COUNT; i++)
		{
			taskHandles[index] = highPriorityTasksPool.Alloc(TaskHigh(i));
			index++;
		}

		for(uint32 i = 0; i < TASK_COUNT; i++)
		{
			taskHandles[index] = normalPriorityTasksPool.Alloc(TaskNormal(i));
			index++;
		}

		switchCountToNormal.Store(0);
		switchCountToLow.Store(0);

		for(uint32 i = 0; i < MT_ARRAY_SIZE(workerState); i++)
		{
			workerState[i].Reset();
		}

		scheduler.RunAsync(MT::TaskGroup::Default(), &taskHandles[0], MT_ARRAY_SIZE(taskHandles));
		CHECK(scheduler.WaitAll(2000));

		int32 workersCount = scheduler.GetWorkersCount();
		float minTasksExecuted  = (float)TASK_COUNT / (float)workersCount;
		minTasksExecuted *= 0.95f;
		uint32 minTasksExecutedThreshold = (uint32)minTasksExecuted;

		uint32 lowProcessedTotal = 0;
		uint32 normalProcessedTotal = 0;
		uint32 highProcessedTotal = 0;

		for(int32 j = 0; j < workersCount; j++)
		{
			lowProcessedTotal += workerState[j].lowProcessed;
			normalProcessedTotal += workerState[j].normalProcessed;
			highProcessedTotal += workerState[j].highProcessed;
		}

		CHECK_EQUAL(TASK_COUNT, lowProcessedTotal);
		CHECK_EQUAL(TASK_COUNT, normalProcessedTotal);
		CHECK_EQUAL(TASK_COUNT, highProcessedTotal);

		for(int32 j = 0; j < workersCount; j++)
		{
			printf("worker #%d\n", j);

			CHECK_EQUAL((uint32)2, workerState[j].taskPrio);
			CHECK(workerState[j].lowProcessed >= minTasksExecutedThreshold);
			CHECK(workerState[j].normalProcessed >= minTasksExecutedThreshold);
			CHECK(workerState[j].highProcessed >= minTasksExecutedThreshold);

			printf("   low : %d\n", workerState[j].lowProcessed);
			printf("   normal : %d\n", workerState[j].normalProcessed);
			printf("   high : %d\n", workerState[j].highProcessed);
		}

		//
		// Every worker thread can't change state more than once.
		//
		CHECK_EQUAL(workersCount, switchCountToNormal.Load());
		CHECK_EQUAL(workersCount, switchCountToLow.Load());

		
	}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}

