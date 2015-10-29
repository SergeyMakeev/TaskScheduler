#include "Tests.h"
#include <UnitTest++.h>
#include <MTScheduler.h>




SUITE(FiberTests)
{
  MT::AtomicInt counter(0);


	MT::Fiber* fiberMain = nullptr;


	void FiberFunc( void* userData )
	{
		CHECK_EQUAL(0, counter.Get());
		counter.Inc();

		MT::Fiber* currentFiber = (MT::Fiber*)userData;
		MT::Fiber::SwitchTo(*currentFiber, *fiberMain);

		CHECK_EQUAL(2, counter.Get());
		counter.Inc();

		MT::Fiber::SwitchTo(*currentFiber, *fiberMain);
	}


	void TestThread(void* userData)
	{
		fiberMain = new MT::Fiber();

		counter.Set(0);

		MT::Thread* thread = (MT::Thread*)userData;

		fiberMain->CreateFromThread(*thread);

		MT::Fiber fiber1;
        
		fiber1.Create(32768, FiberFunc, &fiber1);

		MT::Fiber::SwitchTo(*fiberMain, fiber1);

		CHECK_EQUAL(1, counter.Get());
		counter.Inc();

		MT::Fiber::SwitchTo(*fiberMain, fiber1);

		CHECK_EQUAL(3, counter.Get());

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
