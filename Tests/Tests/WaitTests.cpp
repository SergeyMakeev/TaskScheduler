#include "Tests.h"
#include <UnitTest++.h>
#include <MTScheduler.h>

SUITE(WaitTests)
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace SimpleWaitFromSubtask
{
	MT::AtomicInt32 subTaskCount;
	MT::AtomicInt32 taskCount;

	MT::TaskGroup testGroup;

	struct Subtask : public MT::TaskBase<Subtask>
	{
		MT_DECLARE_DEBUG_INFO("Subtask", MT_COLOR_DEFAULT);

		void Do(MT::FiberContext&)
		{
			MT::Thread::SpinSleep(2);
			subTaskCount.IncFetch();
		}
	};


	struct Task : public MT::TaskBase<Task>
	{
		MT_DECLARE_DEBUG_INFO("Task", MT_COLOR_DEFAULT);

		void Do(MT::FiberContext& ctx)
		{
			Subtask tasks[2];
			ctx.RunAsync(testGroup, &tasks[0], MT_ARRAY_SIZE(tasks));
			ctx.WaitGroupAndYield(testGroup);

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

