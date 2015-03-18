#include "Tests.h"
#include "../../TestFramework/UnitTest++/UnitTest++.h"
#include "../Scheduler.h"
#include "../Platform.h"

SUITE(CleanupTests)
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct NotFinishedTaskDestroy
{
	TASK_METHODS(NotFinishedTaskDestroy)

	static const int timeLimitMS = 100;
	void Do(MT::FiberContext&)
	{
		MT::Thread::Sleep(timeLimitMS * 2);
	}
};

// Checks one simple task
TEST(NotFinishedTaskDestroy)
{
	MT::TaskScheduler scheduler;

	NotFinishedTaskDestroy task;

	scheduler.RunAsync(MT::TaskGroup::GROUP_0, &task, 1);

	CHECK(!scheduler.WaitAll(NotFinishedTaskDestroy::timeLimitMS));
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
