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
		thread.Join();

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

		thread.Join();

	}

	TEST(SleepTest)
	{

		MT::Timer timer;

		MT::Thread::SpinSleep(100);

		CHECK( timer.GetPastMilliSeconds() >= 100 );
	}
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
