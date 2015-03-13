#include "Tests.h"
#include "../../TestFramework/UnitTest++/UnitTest++.h"
#include "../Scheduler.h"

SUITE(SimpleTests)
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace SimpleTask
{
	static int sourceData = 0xFF33FF;
	static int resultData = 0;
	void MT_CALL_CONV Run(MT::FiberContext&, void* userData)
	{
		resultData = *(int*)userData;
	}


	// Checks one simple task
	TEST(RunOneSimpleTask)
	{
		MT::TaskScheduler scheduler;
		MT::TaskDesc task(SimpleTask::Run, &SimpleTask::sourceData);

		scheduler.RunTasks(MT::TaskGroup::GROUP_0, &task, 1);

		CHECK(scheduler.WaitAll(100));
		CHECK_EQUAL(sourceData, resultData);
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace ALotOfTasks
{
	static const int TASK_COUNT = 1000;
	static const int TASK_DURATION_MS = 1;

	void MT_CALL_CONV TaskFunction(MT::FiberContext&, void* userData)
	{
		MT::AtomicInt& counter = *(MT::AtomicInt*)userData;
		counter.Inc();
		Time::SpinSleep(TASK_DURATION_MS * 1000);
	}

	// Checks one simple task
	TEST(ALotOfTasks)
	{
		MT::TaskScheduler scheduler;

		MT::AtomicInt counter;

		MT::TaskDesc tasks[TASK_COUNT];

		for (size_t i = 0; i < ARRAY_SIZE(tasks); ++i)
			tasks[i] = MT::TaskDesc(TaskFunction, &counter);

		scheduler.RunTasks(MT::TaskGroup::GROUP_0, &tasks[0], ARRAY_SIZE(tasks));

		int timeout = (TASK_COUNT * TASK_DURATION_MS / scheduler.GetWorkerCount()) * 2;

		CHECK(scheduler.WaitAll(timeout));
		CHECK_EQUAL(TASK_COUNT, counter.Get());
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}