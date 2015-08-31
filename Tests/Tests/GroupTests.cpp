#include "Tests.h"
#include <UnitTest++.h>
#include <MTScheduler.h>
#include <math.h> // for sin/cos

SUITE(GroupTests)
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace SimpleWaitFromSubtask
{
	struct DummyAnimTask : public MT::TaskBase<DummyAnimTask>
	{
		DECLARE_DEBUG("DummyAnim", MT_COLOR_DEFAULT);

		float tempData[2048];

		void Do(MT::FiberContext& )
		{
			for (int i = 0; i < MT_ARRAY_SIZE(tempData); i++)
			{
				tempData[i] = cos((float)rand() / RAND_MAX);
			}
		}
	};


	struct DummyPhysicTask : public MT::TaskBase<DummyPhysicTask>
	{
		DECLARE_DEBUG("DummyPhysic", MT_COLOR_DEFAULT);

		float tempData[2048];

		void Do(MT::FiberContext& )
		{
			for (int i = 0; i < MT_ARRAY_SIZE(tempData); i++)
			{
				tempData[i] = sin((float)rand() / RAND_MAX);
			}
		}
	};



	TEST(RunGroupTests)
	{
		static uint32 waitTime = 2000;


		MT::TaskScheduler scheduler;

		MT::TaskGroup groupAnim;
		DummyAnimTask dummyAnim[4];
		scheduler.RunAsync(&groupAnim, &dummyAnim[0], MT_ARRAY_SIZE(dummyAnim));

		MT::TaskGroup groupPhysic;
		DummyPhysicTask dummyPhysic[4];
		scheduler.RunAsync(&groupPhysic, &dummyPhysic[0], MT_ARRAY_SIZE(dummyPhysic));

		CHECK(scheduler.WaitGroup(&groupAnim, waitTime));
		CHECK_EQUAL(0, groupAnim.GetTaskCount());

		CHECK(scheduler.WaitGroup(&groupPhysic, waitTime));
		CHECK_EQUAL(0, groupPhysic.GetTaskCount());
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}

