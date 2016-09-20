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

SUITE(WaitTests)
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace SimpleWaitFromSubtask
{
	MT::Atomic32<int32> subTaskCount;
	MT::Atomic32<int32> taskCount;

	MT::TaskGroup testGroup;

	struct Subtask
	{
		MT_DECLARE_TASK(Subtask, MT::StackRequirements::STANDARD, MT::TaskPriority::NORMAL, MT::Color::Blue);

		void Do(MT::FiberContext&)
		{
			MT::SpinSleepMilliSeconds(2);
			subTaskCount.IncFetch();
		}
	};


	struct Task
	{
		MT_DECLARE_TASK(Task, MT::StackRequirements::STANDARD, MT::TaskPriority::NORMAL, MT::Color::Blue);

		void Do(MT::FiberContext& ctx)
		{
			Subtask tasks[2];
			ctx.RunSubtasksAndYield(testGroup, &tasks[0], MT_ARRAY_SIZE(tasks));

			taskCount.IncFetch();

		}
	};


	// Checks one simple task
	TEST(RunOneSimpleWaitTask)
	{
		taskCount.Store(0);
		subTaskCount.Store(0);

		MT::TaskScheduler scheduler;

		testGroup = scheduler.CreateGroup();

		Task tasks[16];
		scheduler.RunAsync(MT::TaskGroup::Default(), &tasks[0], MT_ARRAY_SIZE(tasks));

		CHECK(scheduler.WaitAll(2000));

		int subTaskCountFinisehd = subTaskCount.Load();
		CHECK_EQUAL(MT_ARRAY_SIZE(tasks) * 2, (size_t)subTaskCountFinisehd);

		int taskCountFinished = taskCount.Load();
		CHECK_EQUAL(MT_ARRAY_SIZE(tasks), (size_t)taskCountFinished);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	struct LongTask
	{
		MT_DECLARE_TASK(LongTask, MT::StackRequirements::STANDARD, MT::TaskPriority::NORMAL, MT::Color::Blue);

		void Do(MT::FiberContext&)
		{
			MT::Thread::Sleep(1);
		}
	};

	TEST(TimeoutWaitAllTest)
	{
		MT::TaskScheduler scheduler;

		LongTask tasks[4000];
		scheduler.RunAsync(MT::TaskGroup::Default(), &tasks[0], MT_ARRAY_SIZE(tasks));

		int64 startTime = MT::GetTimeMicroSeconds();

		CHECK_EQUAL(false, scheduler.WaitAll(33));

		int64 endTime = MT::GetTimeMicroSeconds();
		int32 waitTime = (int32)(endTime - startTime);
		printf("WaitAll(33) = %3.2f ms\n", waitTime / 1000.0f);
	}

	TEST(TimeoutWaitGroupTest)
	{
		MT::TaskScheduler scheduler;

		MT::TaskGroup myGroup = scheduler.CreateGroup();

		LongTask tasks[4000];
		scheduler.RunAsync(myGroup, &tasks[0], MT_ARRAY_SIZE(tasks));

		int64 startTime = MT::GetTimeMicroSeconds();

		CHECK_EQUAL(false, scheduler.WaitGroup(myGroup, 33));

		int64 endTime = MT::GetTimeMicroSeconds();
		int32 waitTime = (int32)(endTime - startTime);
		printf("WaitGroup(33) = %3.2f ms\n", waitTime / 1000.0f);
	}


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	MT::Atomic32<uint32> finishedTaskCount;

	struct SecondaryTask
	{
		MT_DECLARE_TASK(SecondaryTask, MT::StackRequirements::STANDARD, MT::TaskPriority::NORMAL, MT::Color::Blue);

		void Do(MT::FiberContext&)
		{
			MT::SpinSleepMicroSeconds(20);
			finishedTaskCount.IncFetch();
		}
	};


	struct PrimaryTask
	{
		MT::TaskGroup secondaryGroup;

		MT_DECLARE_TASK(PrimaryTask, MT::StackRequirements::STANDARD, MT::TaskPriority::NORMAL, MT::Color::Blue);

		PrimaryTask(MT::TaskGroup _secondaryGroup)
			: secondaryGroup(_secondaryGroup)
		{
		}

		void Do(MT::FiberContext& ctx)
		{
			SecondaryTask tasks[64];
			ctx.RunAsync(secondaryGroup, &tasks[0], MT_ARRAY_SIZE(tasks));

			//CHECK(ctx.WaitGroup(secondaryGroup, 10000));

			finishedTaskCount.IncFetch();
		}
	};


	TEST(RunOneSimpleWaitTaskFromTask)
	{
		finishedTaskCount.Store(0);

		MT::TaskScheduler scheduler;

		MT::TaskGroup mainGroup = scheduler.CreateGroup();
		MT::TaskGroup secondaryGroup = scheduler.CreateGroup();

		PrimaryTask task(secondaryGroup);
		scheduler.RunAsync(mainGroup, &task, 1);

		CHECK(scheduler.WaitGroup(mainGroup, 10000));
	}


}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}

