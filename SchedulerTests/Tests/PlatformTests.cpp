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

	MT::Event *pEvent1 = nullptr;
	MT::Event *pEvent2 = nullptr;

	void MyThreadFunc2(void*)
	{
		MT::SpinSleepMilliSeconds(300);
		pEvent1->Signal();
	}

	TEST(EventTest)
	{
		MT::Event event1;
		event1.Create(MT::EventReset::MANUAL, false);
		pEvent1 = &event1;

		MT::Event event2;
		event2.Create(MT::EventReset::AUTOMATIC, false);
		pEvent2 = &event2;

		MT::Thread thread;
		thread.Start(32768, MyThreadFunc2, nullptr);

		bool res0 = event1.Wait(100);
		bool res1 = event1.Wait(1000);
		bool res2 = event1.Wait(1000);
		event1.Reset();
		bool res3 = event1.Wait(100);

		CHECK(!res0);
		CHECK(res1);
		CHECK(res2);
		CHECK(!res3);

		bool res4 = event2.Wait(100);
		CHECK(!res4);
		bool res5 = event2.Wait(100);
		CHECK(!res5);
		bool res6 = event2.Wait(100);
		CHECK(!res6);

		thread.Join();
	}



	MT::AtomicPtrBase<MT::Event> pStressEvent = { nullptr };
	MT::Atomic32Base<uint32> needExitSignal = { 0 };
	MT::Atomic32Base<uint32> needExitWait = { 0 };

	void EventStressTestSignalThreadFunc(void*)
	{
		while (needExitSignal.Load() == 0)
		{
			MT::SpinSleepMicroSeconds(50);
			MT::Event * pEvent = pStressEvent.Load();
			pEvent->Signal();
		}
	}

	void EventStressTestWaitThreadFunc(void*)
	{
		while (needExitWait.Load() == 0)
		{
			MT::Event * pEvent = pStressEvent.Load();
			bool res = pEvent->Wait(1000);
			CHECK(res == true);
		}
	}

	


	TEST(EventStressTest)
	{
		MT::Event stressEvent;
		stressEvent.Create(MT::EventReset::AUTOMATIC, false);
		pStressEvent.Store( &stressEvent );

		needExitSignal.Store(0);
		needExitWait.Store(0);

		MT::Thread signalThreads[6];
		for(uint32 i = 0; i < MT_ARRAY_SIZE(signalThreads); i++)
		{
			signalThreads[i].Start(32768, EventStressTestSignalThreadFunc, nullptr);
		}

		MT::Thread waitThreads[2];
		for(uint32 i = 0; i < MT_ARRAY_SIZE(waitThreads); i++)
		{
			waitThreads[i].Start(32768, EventStressTestWaitThreadFunc, nullptr);
		}



		int64 startTime = MT::GetTimeMicroSeconds();

		const int iterationsCount = 5000;
		for(int i = 0; i < iterationsCount; i++)
		{
			bool res = stressEvent.Wait(1000);
			CHECK(res == true);
		}

		int64 endTime = MT::GetTimeMicroSeconds();

		int microSecondsPerWait = (int)((endTime - startTime) / (int64)iterationsCount);

		printf("microseconds per wait = %d iterations(%d)\n", microSecondsPerWait, iterationsCount);

		needExitWait.Store(1);
		for(uint32 i = 0; i < MT_ARRAY_SIZE(waitThreads); i++)
		{
			waitThreads[i].Join();
		}

		MT::Thread::Sleep(100);

		needExitSignal.Store(1);
		for(uint32 i = 0; i < MT_ARRAY_SIZE(signalThreads); i++)
		{
			signalThreads[i].Join();
		}

		bool res = stressEvent.Wait(300);
		CHECK(res == true);

		res = stressEvent.Wait(300);
		CHECK(res == false);


	}


	TEST(SleepTest)
	{

		MT::Timer timer;

		MT::SpinSleepMilliSeconds(100);

		CHECK( timer.GetPastMilliSeconds() >= 100 );
	}
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
