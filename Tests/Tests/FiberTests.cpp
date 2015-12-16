#include "Tests.h"
#include <UnitTest++.h>
#include <MTScheduler.h>




SUITE(FiberTests)
{
	MT::AtomicInt32 counter(0);
	MT::Fiber* fiberMain = nullptr;

	void FiberFunc( void* userData )
	{
		CHECK_EQUAL(0, counter.Load());
		counter.IncFetch();

		MT::Fiber* currentFiber = (MT::Fiber*)userData;
		MT::Fiber::SwitchTo(*currentFiber, *fiberMain);

		CHECK_EQUAL(2, counter.Load());
		counter.IncFetch();

		MT::Fiber::SwitchTo(*currentFiber, *fiberMain);
	}


	void TestThread(void* userData)
	{
		fiberMain = new MT::Fiber();

		counter.Store(0);

		MT::Thread* thread = (MT::Thread*)userData;

		fiberMain->CreateFromThread(*thread);

		MT::Fiber fiber1;
        
		fiber1.Create(32768, FiberFunc, &fiber1);

		MT::Fiber::SwitchTo(*fiberMain, fiber1);

		CHECK_EQUAL(1, counter.Load());
		counter.IncFetch();

		MT::Fiber::SwitchTo(*fiberMain, fiber1);

		CHECK_EQUAL(3, counter.Load());

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
