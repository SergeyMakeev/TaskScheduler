#include "Tests.h"
#include "../../TestFramework/UnitTest++/UnitTest++.h"
#include "../Scheduler.h"

SUITE(CleanupTests)
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NotFinishedTaskDestroy
{
	static int timeLimitMS = 100;
	void MT_CALL_CONV Run(MT::FiberContext&, void*)
	{
		Sleep(timeLimitMS * 2);
	}
}

// Checks one simple task
TEST(NotFinishedTaskDestroy)
{
	MT::TaskScheduler scheduler;
	MT::TaskDesc task(NotFinishedTaskDestroy::Run, nullptr);

	scheduler.RunTasks(MT::TaskGroup::GROUP_0, &task, 1);

	CHECK(!scheduler.WaitAll(NotFinishedTaskDestroy::timeLimitMS));
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}