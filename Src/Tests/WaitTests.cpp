#include "Tests.h"
#include "../../TestFramework/UnitTest++/UnitTest++.h"
#include "../Scheduler.h"

/*

SUITE(WaitTests)
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace SimpleWaitFromSubtask
{
	void MT_CALL_CONV RunSubtask(MT::FiberContext&, void*)
	{
		Sleep(1);
	}


	void MT_CALL_CONV Run(MT::FiberContext& ctx, void*)
	{
		MT::TaskDesc tasks[2];
		for (int i = 0; i < ARRAY_SIZE(tasks); i++)
		{
			tasks[i] = MT::TaskDesc(SimpleWaitFromSubtask::RunSubtask, nullptr);
		}

		ctx.RunAsync(&tasks[0], ARRAY_SIZE(tasks));

		ctx.WaitAllAndYield(10000);
	}


	// Checks one simple task
	TEST(RunOneSimpleTask)
	{
		MT::TaskScheduler scheduler;

		MT::TaskDesc tasks[128];
		for (int i = 0; i < ARRAY_SIZE(tasks); i++)
		{
			tasks[i] = MT::TaskDesc(SimpleWaitFromSubtask::Run, nullptr);
		}

		scheduler.RunAsync(MT::TaskGroup::GROUP_0, &tasks[0], ARRAY_SIZE(tasks));

		CHECK(scheduler.WaitAll(1000));
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}

*/