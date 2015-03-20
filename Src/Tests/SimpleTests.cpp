#include "Tests.h"
#include "../../TestFramework/UnitTest++/UnitTest++.h"
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

	int GetSourceData()
	{
		return sourceData;
	}
};

// Checks one simple task
TEST(RunOneSimpleTask)
{
	MT::TaskScheduler scheduler;

	SimpleTask task;
	scheduler.RunAsync(MT::TaskGroup::GROUP_0, &task, 1);

	CHECK(scheduler.WaitAll(100));
	CHECK_EQUAL(task.GetSourceData(), task.resultData);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct ALotOfTasks
{
	TASK_METHODS(ALotOfTasks)

	MT::AtomicInt* counter;

	void Do(MT::FiberContext&)
	{
		counter->Inc();
		MT::Thread::SpinSleep(1);
	}
};

// Checks one simple task
TEST(ALotOfTasks)
{
	MT::TaskScheduler scheduler;

	MT::AtomicInt counter;

	static const int TASK_COUNT = 1000;

	ALotOfTasks tasks[TASK_COUNT];

	for (size_t i = 0; i < ARRAY_SIZE(tasks); ++i)
		tasks[i].counter = &counter;

	scheduler.RunAsync(MT::TaskGroup::GROUP_0, &tasks[0], ARRAY_SIZE(tasks));

	int timeout = (TASK_COUNT / scheduler.GetWorkerCount()) * 4;

	CHECK(scheduler.WaitGroup(MT::TaskGroup::GROUP_0, timeout));
	CHECK_EQUAL(TASK_COUNT, counter.Get());
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
