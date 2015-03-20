#include "Tests.h"
#include "../../TestFramework/UnitTest++/UnitTest++.h"
#include "../Scheduler.h"



SUITE(WaitTests)
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace SimpleWaitFromSubtask
{
	MT::AtomicInt subTaskCount;
	MT::AtomicInt taskCount;

	struct Subtask
	{
		void Do(MT::FiberContext&)
		{
			MT::Thread::SpinSleep(2);
			subTaskCount.Inc();
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

			taskCount.Inc();
		}

		TASK_METHODS(Task);
	};


	// Checks one simple task
	TEST(RunOneSimpleTask)
	{
		taskCount.Set(0);
		subTaskCount.Set(0);

		MT::TaskScheduler scheduler;

		Task tasks[16];
		scheduler.RunAsync(MT::TaskGroup::GROUP_0, &tasks[0], ARRAY_SIZE(tasks));

		CHECK(scheduler.WaitAll(2000));

		int subTaskCountFinisehd = subTaskCount.Get();
		CHECK(subTaskCountFinisehd == ARRAY_SIZE(tasks) * 2);

		int taskCountFinished = taskCount.Get();
		CHECK(taskCountFinished == ARRAY_SIZE(tasks));
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}

