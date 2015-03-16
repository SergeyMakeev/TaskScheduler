#include "Tests.h"
#include "../../TestFramework/UnitTest++/UnitTest++.h"
#include "../../TestFramework/UnitTest++/UnitTestTimer.h"
#include "../Scheduler.h"

SUITE(SimpleTests)
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SimpleTask
{
	TASK_METHODS(SimpleTask)

	static const int sourceData = 0xFF33FF;
	int resultData;

	SimpleTask() : resultData(0) {}

	void Do(MT::FiberContext&)
	{
		resultData = sourceData;
	}
};

// Checks one simple task
TEST(RunOneSimpleTask)
{
	MT::TaskScheduler scheduler;

	SimpleTask task;
	scheduler.RunAsync(MT::TaskGroup::GROUP_0, &task, 1);

	CHECK(scheduler.WaitAll(100));
	CHECK_EQUAL(task.sourceData, task.resultData);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct ALotOfTasks
{
	TASK_METHODS(ALotOfTasks)

	static const int TASK_COUNT = 1000;
	static const int TASK_DURATION_MS = 1;

	MT::AtomicInt* counter;

	void Do(MT::FiberContext&)
	{
		counter->Inc();
		Time::SpinSleep(TASK_DURATION_MS * 1000);
	}
};

// Checks one simple task
TEST(ALotOfTasks)
{
	MT::TaskScheduler scheduler;

	MT::AtomicInt counter;

	ALotOfTasks tasks[ALotOfTasks::TASK_COUNT];

	for (size_t i = 0; i < ARRAY_SIZE(tasks); ++i)
		tasks[i].counter = &counter;

	scheduler.RunAsync(MT::TaskGroup::GROUP_0, &tasks[0], ARRAY_SIZE(tasks));

	int timeout = (ALotOfTasks::TASK_COUNT * ALotOfTasks::TASK_DURATION_MS / scheduler.GetWorkerCount()) * 2;

	CHECK(scheduler.WaitGroup(MT::TaskGroup::GROUP_0, timeout));
	CHECK_EQUAL(ALotOfTasks::TASK_COUNT, counter.Get());
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}