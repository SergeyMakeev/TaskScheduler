#include "Tests.h"
#include <UnitTest++.h>
#include <MTScheduler.h>

SUITE(SimpleTests)
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SimpleTask : public MT::TaskBase<SimpleTask>
{
	DECLARE_DEBUG("SimpleTask", MT_COLOR_DEFAULT);

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
struct ALotOfTasks : public MT::TaskBase<ALotOfTasks>
{
	DECLARE_DEBUG("ALotOfTasks", MT_COLOR_DEFAULT);

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

	for (size_t i = 0; i < MT_ARRAY_SIZE(tasks); ++i)
		tasks[i].counter = &counter;

	scheduler.RunAsync(MT::TaskGroup::GROUP_0, &tasks[0], MT_ARRAY_SIZE(tasks));

	int timeout = (TASK_COUNT / scheduler.GetWorkerCount()) * 200;

	CHECK(scheduler.WaitGroup(MT::TaskGroup::GROUP_0, timeout));
	CHECK_EQUAL(TASK_COUNT, counter.Get());
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
