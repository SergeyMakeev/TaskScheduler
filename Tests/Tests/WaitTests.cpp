#include "Tests.h"
#include <UnitTest++.h>
#include <MTScheduler.h>

SUITE(WaitTests)
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace SimpleWaitFromSubtask
{
	MT::AtomicInt subTaskCount;
	MT::AtomicInt taskCount;

	MT::TaskGroup testGroup;

	struct Subtask : public MT::TaskBase<Subtask>
	{
		MT_DECLARE_DEBUG_INFO("Subtask", MT_COLOR_DEFAULT);

		void Do(MT::FiberContext&)
		{
			MT::Thread::SpinSleep(2);
			subTaskCount.Inc();
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

			taskCount.Inc();

		}
	};


	// Checks one simple task
	TEST(RunOneSimpleWaitTask)
	{
		taskCount.Set(0);
		subTaskCount.Set(0);

		MT::TaskScheduler scheduler;

		testGroup = scheduler.CreateGroup();

		Task tasks[16];
		scheduler.RunAsync(MT::TaskGroup::Default(), &tasks[0], MT_ARRAY_SIZE(tasks));

		CHECK(scheduler.WaitAll(2000));

		int subTaskCountFinisehd = subTaskCount.Get();
		CHECK(subTaskCountFinisehd == MT_ARRAY_SIZE(tasks) * 2);

		int taskCountFinished = taskCount.Get();
		CHECK(taskCountFinished == MT_ARRAY_SIZE(tasks));
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}

