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
			MT::Thread::SpinSleepMilliSeconds(2);
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
		CHECK(subTaskCountFinisehd == MT_ARRAY_SIZE(tasks) * 2);

		int taskCountFinished = taskCount.Load();
		CHECK(taskCountFinished == MT_ARRAY_SIZE(tasks));
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}

