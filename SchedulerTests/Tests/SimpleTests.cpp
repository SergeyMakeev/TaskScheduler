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

SUITE(SimpleTests)
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SimpleTask
{
	MT_DECLARE_TASK(SimpleTask, MT::StackRequirements::STANDARD, MT::Color::Blue);

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
	MT_DECLARE_TASK(ALotOfTasks, MT::StackRequirements::STANDARD, MT::Color::Blue);

	MT::Atomic32<int32>* counter;

	void Do(MT::FiberContext&)
	{
		counter->IncFetch();
		MT::Thread::SpinSleepMilliSeconds(1);
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
}
