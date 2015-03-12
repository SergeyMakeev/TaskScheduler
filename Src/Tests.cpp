#include "Tests.h"
#include "../Test/UnitTest++/UnitTest++.h"

#include "Scheduler.h"


namespace SimpleTask
{
	static int sourceData = 0xFF33FF;
	static int resultData = 0;
	void MT_CALL_CONV Run(MT::FiberContext& context, void* userData)
	{
		resultData = *(int*)userData;
	}
}

// Checks one simple task
TEST(RunOneSimpleTask)
{
	MT::TaskScheduler scheduler;
	MT::TaskDesc task(SimpleTask::Run, &SimpleTask::sourceData);

	scheduler.RunTasks(MT::TaskGroup::GROUP_0, &task, 1);

	CHECK(scheduler.WaitAll(100));
	CHECK_EQUAL(SimpleTask::sourceData, SimpleTask::resultData);
}

//namespace LongTask
//{
//	static int timeLimitMS = 100;
//	void MT_CALL_CONV Run(MT::FiberContext& context, void* userData)
//	{
//		Sleep(timeLimitMS * 2);
//	}
//}
//
//// Checks one simple task
//TEST(RunLongTaskAndNotFinish)
//{
//	MT::TaskScheduler scheduler;
//	MT::TaskDesc task(LongTask::Run, nullptr);
//
//	scheduler.RunTasks(MT::TaskGroup::GROUP_0, &task, 1);
//
//	CHECK(!scheduler.WaitAll(LongTask::timeLimitMS));
//}

int Tests::RunAll()
{
	return UnitTest::RunAllTests();
}
