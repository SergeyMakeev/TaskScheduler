#include "Tests.h"
#include <UnitTest++.h>
#include <MTScheduler.h>

SUITE(PlatformTests)
{
	static const intptr_t DATA_VALUE = 13;

	intptr_t g_Variable = 0;

	void MyThreadFunc(void* userData)
	{
		intptr_t data = (intptr_t)userData;

		CHECK(data == DATA_VALUE);

		g_Variable = data;
	}

	TEST(ThreadTest)
	{
		intptr_t data = DATA_VALUE;

		MT::Thread thread;
		thread.Start(32768, MyThreadFunc, (void*)data);
		thread.Stop();

		CHECK(g_Variable == DATA_VALUE);
	}
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	MT::Event *pEvent = nullptr;

	void MyThreadFunc2(void*)
	{
		MT::Thread::SpinSleep(300);
		pEvent->Signal();
	}

	TEST(EventTest)
	{
		MT::Event event;
		event.Create(MT::EventReset::MANUAL, false);
		pEvent = &event;

		MT::Thread thread;
		thread.Start(32768, MyThreadFunc2, nullptr);

		bool res0 = event.Wait(100);
		bool res1 = event.Wait(1000);

		CHECK(!res0);
		CHECK(res1);

		thread.Stop();

	}

	TEST(SleepTest)
	{

		MT::Timer timer;

		MT::Thread::SpinSleep(100);

		CHECK( timer.GetPastMilliSeconds() >= 100 );
	}
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
