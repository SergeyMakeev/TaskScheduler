// The MIT License (MIT)
//
// 	Copyright (c) 2015 Sergey Makeev, Vadim Slyusarev
//
// 	Permission is hereby granted, free of charge, to any person obtaining a copy
// 	of this software and associated documentation files (the "Software"), to deal
// 	in the Software without restriction, including without limitation the rights
// 	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// 	copies of the Software, and to permit persons to whom the Software is
// 	furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
// 	all copies or substantial portions of the Software.
//
// 	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// 	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// 	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// 	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// 	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// 	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// 	THE SOFTWARE.

#include "Tests.h"
#include <UnitTest++.h>
#include <MTScheduler.h>

#ifdef MT_THREAD_SANITIZER
	#define SMALLEST_STACK_SIZE (566656)
#else
	#define SMALLEST_STACK_SIZE (32768)
#endif


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

	void FiberMain(void* userData)
	{
		MT_UNUSED(userData);

		MT::Fiber fiber1;

		fiber1.Create(SMALLEST_STACK_SIZE, FiberFunc, &fiber1);

		MT::Fiber::SwitchTo(*fiberMain, fiber1);

		CHECK_EQUAL(1, counter.Load());
		counter.IncFetch();

		MT::Fiber::SwitchTo(*fiberMain, fiber1);

		CHECK_EQUAL(3, counter.Load());

		fiberMain = nullptr;
	}

	void TestThread(void* userData)
	{
		MT::Fiber fiber;

		fiberMain = &fiber;

		counter.Store(0);

		MT::Thread* thread = (MT::Thread*)userData;

		fiberMain->CreateFromCurrentThreadAndRun(*thread, FiberMain, userData);
	}



TEST(FiberSimpleTest)
{
	MT::Thread thread;
	thread.Start(SMALLEST_STACK_SIZE, TestThread, &thread);

	thread.Stop();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
