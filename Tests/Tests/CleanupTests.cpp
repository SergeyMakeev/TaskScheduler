#include "Tests.h"
#include <UnitTest++.h>
#include <MTScheduler.h>


SUITE(CleanupTests)
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct NotFinishedTaskDestroy : public MT::TaskBase<NotFinishedTaskDestroy>
{
	DECLARE_DEBUG("NotFinishedTaskDestroy", MT_COLOR_DEFAULT);

	static const int timeLimitMS = 100;
	void Do(MT::FiberContext&)
	{
		MT::Thread::SpinSleep(timeLimitMS * 2);
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
