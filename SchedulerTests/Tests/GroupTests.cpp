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
#include <math.h> // for sin/cos

SUITE(GroupTests)
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace SimpleWaitFromSubtask
{
	MT::Atomic32<int32> subtaskCount(0);
	MT::Atomic32<int32> animTaskCount(0);
	MT::Atomic32<int32> physTaskCount(0);


	struct DummySubTask
	{
		MT_DECLARE_TASK(DummySubTask, MT::StackRequirements::STANDARD, MT::TaskPriority::NORMAL, MT::Color::Blue);

		float tempData[64];

		void Do(MT::FiberContext& )
		{
			for (size_t i = 0; i < MT_ARRAY_SIZE(tempData); i++)
			{
				tempData[i] = cos((float)rand() / RAND_MAX) + sin((float)rand() / RAND_MAX);
			}

			subtaskCount.IncFetch();
		}
	};


	struct DummyAnimTask
	{
		MT_DECLARE_TASK(DummyAnimTask, MT::StackRequirements::STANDARD, MT::TaskPriority::NORMAL, MT::Color::Blue);

		float tempData[128];

		void Do(MT::FiberContext& ctx)
		{
			for (size_t i = 0; i < MT_ARRAY_SIZE(tempData); i++)
			{
				tempData[i] = cos((float)rand() / RAND_MAX);
			}

			DummySubTask subtasks[16];
			ctx.RunSubtasksAndYield(MT::TaskGroup::Default(), &subtasks[0], MT_ARRAY_SIZE(subtasks));

			animTaskCount.IncFetch();
		}
	};


	struct DummyPhysicTask
	{
		MT_DECLARE_TASK(DummyPhysicTask, MT::StackRequirements::STANDARD, MT::TaskPriority::NORMAL, MT::Color::Blue);

		float tempData[128];

		void Do(MT::FiberContext& ctx)
		{
			for (size_t i = 0; i < MT_ARRAY_SIZE(tempData); i++)
			{
				tempData[i] = sin((float)rand() / RAND_MAX);
			}

			DummySubTask subtasks[8];
			ctx.RunSubtasksAndYield(MT::TaskGroup::Default(), &subtasks[0], MT_ARRAY_SIZE(subtasks));

			physTaskCount.IncFetch();
		}
	};



	TEST(RunGroupTests)
	{
		static uint32 waitTime = 50000;

		subtaskCount.Store(0);
		animTaskCount.Store(0);
		physTaskCount.Store(0);

		MT::TaskScheduler scheduler;

		MT::TaskGroup groupEmpty = scheduler.CreateGroup();
		MT::TaskGroup groupAnim = scheduler.CreateGroup();


		bool emptyWaitRes0 = scheduler.WaitGroup(groupEmpty, 200);
		CHECK(emptyWaitRes0 == true);

		DummyAnimTask dummyAnim[4];
		scheduler.RunAsync(groupAnim, &dummyAnim[0], MT_ARRAY_SIZE(dummyAnim));

		MT::TaskGroup groupPhysic = scheduler.CreateGroup();
		DummyPhysicTask dummyPhysic[4];
		scheduler.RunAsync(groupPhysic, &dummyPhysic[0], MT_ARRAY_SIZE(dummyPhysic));

		bool emptyWaitRes1 = scheduler.WaitGroup(groupEmpty, 20);
		CHECK(emptyWaitRes1 == true);

		CHECK(scheduler.WaitGroup(groupAnim, waitTime));

		bool emptyWaitRes2 = scheduler.WaitGroup(groupEmpty, 200);
		CHECK(emptyWaitRes2 == true);


		CHECK_EQUAL(4, animTaskCount.Load());
		CHECK(subtaskCount.Load() >= 4 * 16);


		CHECK(scheduler.WaitGroup(groupPhysic, waitTime));

		CHECK_EQUAL(4, animTaskCount.Load());
		CHECK_EQUAL(4, physTaskCount.Load());
		CHECK_EQUAL(96, subtaskCount.Load());
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}

