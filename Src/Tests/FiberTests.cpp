#include "Tests.h"
#include "../../TestFramework/UnitTest++/UnitTest++.h"
#include "../Scheduler.h"
#include "../Platform.h"




SUITE(FiberTests)
{
	int counter = 0;


	MT::Fiber* fiberMain = nullptr;


	void FiberFunc( void* userData )
	{
		CHECK_EQUAL(counter, 0);
		counter++;

		MT::Fiber* currentFiber = (MT::Fiber*)userData;
		MT::Fiber::SwitchTo(*currentFiber, *fiberMain);

		CHECK_EQUAL(counter, 2);
		counter++;

		MT::Fiber::SwitchTo(*currentFiber, *fiberMain);
	}


	void TestThread(void* userData)
	{
		fiberMain = new MT::Fiber();

		counter = 0;

		MT::Thread* thread = (MT::Thread*)userData;

		fiberMain->CreateFromThread(*thread);

		MT::Fiber fiber1;
		fiber1.Create(16384, FiberFunc, &fiber1);

		MT::Fiber::SwitchTo(*fiberMain, fiber1);

		CHECK_EQUAL(counter, 1);
		counter++;

		MT::Fiber::SwitchTo(*fiberMain, fiber1);

		CHECK_EQUAL(counter, 3);

		delete fiberMain;
		fiberMain = nullptr;
	}




TEST(FiberSimpleTest)
{
	MT::Thread thread;
	thread.Start(32768, TestThread, &thread);

	thread.Stop();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
