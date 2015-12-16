#include "Tests.h"
#include <UnitTest++.h>
#include <MTScheduler.h>

SUITE(SimpleTests)
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SimpleTask : public MT::TaskBase<SimpleTask>
{
	MT_DECLARE_DEBUG_INFO("SimpleTask", MT_COLOR_DEFAULT);

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
	scheduler.RunAsync(MT::TaskGroup::Default(), &task, 1);

	CHECK(scheduler.WaitAll(1000));
	CHECK_EQUAL(task.GetSourceData(), task.resultData);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct ALotOfTasks : public MT::TaskBase<ALotOfTasks>
{
	MT_DECLARE_DEBUG_INFO("ALotOfTasks", MT_COLOR_DEFAULT);

	MT::AtomicInt32* counter;

	void Do(MT::FiberContext&)
	{
		counter->IncFetch();
		MT::Thread::SpinSleep(1);
	}
};

// Checks one simple task
TEST(ALotOfTasks)
{

	MT::TaskScheduler scheduler;

	MT::AtomicInt32 counter;

	static const int TASK_COUNT = 1000;

	ALotOfTasks tasks[TASK_COUNT];

	for (size_t i = 0; i < MT_ARRAY_SIZE(tasks); ++i)
		tasks[i].counter = &counter;

	scheduler.RunAsync(MT::TaskGroup::Default(), &tasks[0], MT_ARRAY_SIZE(tasks));

	int timeout = (TASK_COUNT / scheduler.GetWorkerCount()) * 2000;

	CHECK(scheduler.WaitGroup(MT::TaskGroup::Default(), timeout));
	CHECK_EQUAL(TASK_COUNT, counter.Load());
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
