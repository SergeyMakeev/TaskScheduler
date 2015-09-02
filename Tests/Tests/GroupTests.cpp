#include "Tests.h"
#include <UnitTest++.h>
#include <MTScheduler.h>
#include <math.h> // for sin/cos

SUITE(GroupTests)
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace SimpleWaitFromSubtask
{
	MT::AtomicInt subtaskCount(0);
	MT::AtomicInt animTaskCount(0);
	MT::AtomicInt physTaskCount(0);


	struct DummySubTask : public MT::TaskBase<DummySubTask>
	{
		DECLARE_DEBUG("DummySubtask", MT_COLOR_DEFAULT);

		float tempData[2048];

		void Do(MT::FiberContext& )
		{
			for (size_t i = 0; i < MT_ARRAY_SIZE(tempData); i++)
			{
				tempData[i] = cos((float)rand() / RAND_MAX) + sin((float)rand() / RAND_MAX);
			}

			subtaskCount.Inc();
		}
	};


	struct DummyAnimTask : public MT::TaskBase<DummyAnimTask>
	{
		DECLARE_DEBUG("DummyAnim", MT_COLOR_DEFAULT);

		float tempData[2048];

		void Do(MT::FiberContext& ctx)
		{
			for (size_t i = 0; i < MT_ARRAY_SIZE(tempData); i++)
			{
				tempData[i] = cos((float)rand() / RAND_MAX);
			}

			DummySubTask subtasks[16];
			ctx.RunAsync(MT::DEFAULT_GROUP, &subtasks[0], MT_ARRAY_SIZE(subtasks));
			ctx.WaitGroupAndYield(MT::DEFAULT_GROUP);

			animTaskCount.Inc();
		}
	};


	struct DummyPhysicTask : public MT::TaskBase<DummyPhysicTask>
	{
		DECLARE_DEBUG("DummyPhysic", MT_COLOR_DEFAULT);

		float tempData[2048];

		void Do(MT::FiberContext& ctx)
		{
			for (size_t i = 0; i < MT_ARRAY_SIZE(tempData); i++)
			{
				tempData[i] = sin((float)rand() / RAND_MAX);
			}

			DummySubTask subtasks[8];
			ctx.RunSubtasksAndYield(MT::DEFAULT_GROUP, &subtasks[0], MT_ARRAY_SIZE(subtasks));

			physTaskCount.Inc();
		}
	};



	TEST(RunGroupTests)
	{
		static uint32 waitTime = 50000;

		subtaskCount.Set(0);
		animTaskCount.Set(0);
		physTaskCount.Set(0);

		MT::TaskScheduler scheduler;

		MT::TaskGroup groupAnim = scheduler.CreateGroup();
		DummyAnimTask dummyAnim[4];
		scheduler.RunAsync(groupAnim, &dummyAnim[0], MT_ARRAY_SIZE(dummyAnim));

		MT::TaskGroup groupPhysic = scheduler.CreateGroup();
		DummyPhysicTask dummyPhysic[4];
		scheduler.RunAsync(groupPhysic, &dummyPhysic[0], MT_ARRAY_SIZE(dummyPhysic));

		CHECK(scheduler.WaitGroup(groupAnim, waitTime));

		CHECK_EQUAL(4, animTaskCount.Get());
		CHECK(subtaskCount.Get() >= 4 * 16);
		

		CHECK(scheduler.WaitGroup(groupPhysic, waitTime));

		CHECK_EQUAL(4, animTaskCount.Get());
		CHECK_EQUAL(4, physTaskCount.Get());
		CHECK_EQUAL(96, subtaskCount.Get());
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}

