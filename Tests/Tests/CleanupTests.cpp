#include "Tests.h"
#include <UnitTest++.h>
#include <MTScheduler.h>


SUITE(CleanupTests)
{

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct NotFinishedTaskDestroy
{
	MT_DECLARE_TASK(NotFinishedTaskDestroy, MT::Color::Blue);

	static const int timeLimitMS = 100;
	void Do(MT::FiberContext&)
	{
		MT::Thread::SpinSleep(timeLimitMS * 20);
	}
};

// Checks one simple task
TEST(NotFinishedTaskDestroy)
{
	MT::TaskScheduler scheduler;

	NotFinishedTaskDestroy task;

	scheduler.RunAsync(MT::TaskGroup::Default(), &task, 1);

	CHECK(!scheduler.WaitAll(NotFinishedTaskDestroy::timeLimitMS));
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
