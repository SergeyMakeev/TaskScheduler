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
	static int TASK_COUNT = 1000;

	void MT_CALL_CONV TaskFunction(MT::FiberContext&, void* userData)
	{
		MT::AtomicInt& counter = *(MT::AtomicInt*)userData;
		counter.Inc();
		Sleep(1);
	}

	// Checks one simple task
	TEST(ALotOfTasks)
	{
		MT::TaskScheduler scheduler;

		std::vector<MT::TaskDesc> tasks;
		tasks.resize(TASK_COUNT);

		MT::AtomicInt counter;

		for (size_t i = 0; i < tasks.size(); ++i)
			tasks[i] = MT::TaskDesc(TaskFunction, &counter);

		scheduler.RunTasks(MT::TaskGroup::GROUP_0, &tasks.front(), tasks.size());

		CHECK(scheduler.WaitAll(1000));
		
		CHECK_EQUAL(TASK_COUNT, counter.Get());
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}