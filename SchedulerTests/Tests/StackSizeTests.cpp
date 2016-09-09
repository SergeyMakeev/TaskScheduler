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

SUITE(StackSizeTests)
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct StandartStackSizeTask
{
	MT_DECLARE_TASK(StandartStackSizeTask, MT::StackRequirements::STANDARD, MT::TaskPriority::NORMAL, MT::Color::Blue);

	void Do(MT::FiberContext&)
	{
		//byte stackData[28000];

		// Looks like OSX ASAN took too many stack space
		byte stackData[20000]; 
		for (uint32 i = 0; i < MT_ARRAY_SIZE(stackData); i++)
		{
			stackData[i] = 0x0D;
		}
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct ExtendedStackSizeTask
{
	MT_DECLARE_TASK(ExtendedStackSizeTask, MT::StackRequirements::EXTENDED, MT::TaskPriority::NORMAL, MT::Color::Red);

	void Do(MT::FiberContext&)
	{
		byte stackData[262144];
		for (uint32 i = 0; i < MT_ARRAY_SIZE(stackData); i++)
		{
			stackData[i] = 0x0D;
		}
	}

};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST(RunStandartTasks)
{
	MT::TaskScheduler scheduler;

	StandartStackSizeTask tasks[100];
	scheduler.RunAsync(MT::TaskGroup::Default(), &tasks[0], MT_ARRAY_SIZE(tasks));
	CHECK(scheduler.WaitAll(1000));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST(RunExtendedTasks)
{
	MT::TaskScheduler scheduler;

	ExtendedStackSizeTask tasks[100];
	scheduler.RunAsync(MT::TaskGroup::Default(), &tasks[0], MT_ARRAY_SIZE(tasks));
	CHECK(scheduler.WaitAll(1000));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST(RunMixedTasks)
{
	MT::TaskScheduler scheduler;

	MT::TaskPool<ExtendedStackSizeTask, 64> extendedTaskPool;
	MT::TaskPool<StandartStackSizeTask, 64> standardTaskPool;

	MT::TaskHandle taskHandles[100];
	for (size_t i = 0; i < MT_ARRAY_SIZE(taskHandles); ++i)
	{
		MT::TaskHandle handle;

		if (i & 1)
		{
			handle = extendedTaskPool.Alloc(ExtendedStackSizeTask());
		} else
		{
			handle = standardTaskPool.Alloc(StandartStackSizeTask());
		}
		taskHandles[i] = handle;
	}

	scheduler.RunAsync(MT::TaskGroup::Default(), &taskHandles[0], MT_ARRAY_SIZE(taskHandles));
	CHECK(scheduler.WaitAll(1000));
}



}
