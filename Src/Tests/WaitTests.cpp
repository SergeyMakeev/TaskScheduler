#include "Tests.h"
#include "../../TestFramework/UnitTest++/UnitTest++.h"
#include "../Scheduler.h"



SUITE(WaitTests)
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace SimpleWaitFromSubtask
{
	struct Subtask
	{
		void Do(MT::FiberContext&)
		{
			Sleep(1);
		}

		TASK_METHODS(Subtask);
	};


	struct Task
	{
		void Do(MT::FiberContext& ctx)
		{
			Subtask tasks[2];
			ctx.RunAsync(MT::TaskGroup::GROUP_2, &tasks[0], ARRAY_SIZE(tasks));

			ctx.WaitGroupAndYield(MT::TaskGroup::GROUP_2);
		}

		TASK_METHODS(Task);
	};


	// Checks one simple task
	TEST(RunOneSimpleTask)
	{
		MT::TaskScheduler scheduler;

		Task tasks[128];
		scheduler.RunAsync(MT::TaskGroup::GROUP_0, &tasks[0], ARRAY_SIZE(tasks));

		CHECK(scheduler.WaitAll(1000));
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}

