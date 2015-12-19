#include "Tests.h"
#include <UnitTest++.h>
#include <MTScheduler.h>
#include <math.h> // for sin/cos

SUITE(GroupTests)
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace SimpleWaitFromSubtask
{
	MT::AtomicInt32 subtaskCount(0);
	MT::AtomicInt32 animTaskCount(0);
	MT::AtomicInt32 physTaskCount(0);


	struct DummySubTask
	{
		MT_DECLARE_TASK(DummySubTask, MT_COLOR_DEFAULT);

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
		MT_DECLARE_TASK(DummyAnimTask, MT_COLOR_DEFAULT);

		float tempData[128];

		void Do(MT::FiberContext& ctx)
		{
			for (size_t i = 0; i < MT_ARRAY_SIZE(tempData); i++)
			{
				tempData[i] = cos((float)rand() / RAND_MAX);
			}

			DummySubTask subtasks[16];
			ctx.RunAsync(MT::TaskGroup::Default(), &subtasks[0], MT_ARRAY_SIZE(subtasks));
			ctx.WaitGroupAndYield(MT::TaskGroup::Default());

			animTaskCount.IncFetch();
		}
	};


	struct DummyPhysicTask
	{
		MT_DECLARE_TASK(DummyPhysicTask, MT_COLOR_DEFAULT);

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

		MT::TaskGroup groupAnim = scheduler.CreateGroup();
		DummyAnimTask dummyAnim[4];
		scheduler.RunAsync(groupAnim, &dummyAnim[0], MT_ARRAY_SIZE(dummyAnim));

		MT::TaskGroup groupPhysic = scheduler.CreateGroup();
		DummyPhysicTask dummyPhysic[4];
		scheduler.RunAsync(groupPhysic, &dummyPhysic[0], MT_ARRAY_SIZE(dummyPhysic));

		CHECK(scheduler.WaitGroup(groupAnim, waitTime));

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

