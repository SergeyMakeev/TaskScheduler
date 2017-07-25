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
#include "../Profiler/Profiler.h"
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
	MT::Atomic32Base<uint32> needStartWork = { 0 };

	void EventStressTestSignalThreadFunc(void*)
	{ BROFILER_THREAD("SignalThread");

		while (needStartWork.Load() == 0)
		{ BROFILER_CATEGORY("Signal prepare", 0xFFFF00FF);
			MT::SpinSleepMicroSeconds(300);
		}
	
		while (needExitSignal.Load() == 0)
		{ BROFILER_CATEGORY("Signal Loop", 0xFF556B2F);
			MT::SpinSleepMicroSeconds(300);
			MT::Event * pEvent = pStressEvent.Load();
			pEvent->Signal();
		}
	}

	void EventStressTestWaitThreadFunc(void*)
	{ BROFILER_THREAD("WaitThread");

		while (needStartWork.Load() == 0)
		{ BROFILER_CATEGORY("Wait prepare", 0xFFFF00FF);
			MT::SpinSleepMicroSeconds(300);
		}

		while (needExitWait.Load() == 0)
		{ BROFILER_CATEGORY("Wait Loop", 0xFFF0F8FF);
			MT::Event* pEvent = pStressEvent.Load();
			bool res = pEvent->Wait(1000);
			MT::SpinSleepMicroSeconds(300);
			CHECK(res == true);
		}
	}


	


	TEST(EventStressTest)
	{
#if defined(MT_ENABLE_LEGACY_WINDOWSXP_SUPPORT) && defined(MT_PLATFORM_WINDOWS)
		printf("Kernel mode events\n");
#else
		printf("User mode events\n");
#endif

	
		MT::Event stressEvent;
		MT::Thread signalThreads[3];
		MT::Thread waitThreads[3];
		needStartWork.Store(0);

		{ 
			BROFILER_NEXT_FRAME();
			stressEvent.Create(MT::EventReset::AUTOMATIC, false);
			pStressEvent.Store( &stressEvent );

			needExitSignal.Store(0);
			needExitWait.Store(0);

			for(uint32 i = 0; i < MT_ARRAY_SIZE(signalThreads); i++)
			{
				signalThreads[i].Start(32768, EventStressTestSignalThreadFunc, nullptr);
			}

			for(uint32 i = 0; i < MT_ARRAY_SIZE(waitThreads); i++)
			{
				waitThreads[i].Start(32768, EventStressTestWaitThreadFunc, nullptr);
			}

			printf("Signal threads num = %d\n", (uint32)MT_ARRAY_SIZE(signalThreads));
			printf("Wait threads num = %d\n", (uint32)MT_ARRAY_SIZE(waitThreads));
		}


#if defined(MT_INSTRUMENTED_BUILD) && defined(MT_ENABLE_BROFILER_SUPPORT)
		BROFILER_FRAME("EventStressTest");
		{
			printf("Waiting for 'Brofiler' connection.\n");
			for(;;)
			{
				BROFILER_NEXT_FRAME();
				MT::Thread::Sleep(150);
				if (Brofiler::IsActive())
				{
					break;
				}
			}
		}
#endif
		needStartWork.Store(1);

		const int iterationsCount = 5000;

		int64 startTimeSignal = MT::GetTimeMicroSeconds();
		{ BROFILER_NEXT_FRAME();
		  BROFILER_CATEGORY("Signal Loop", 0xFF556B2F);
			for(int i = 0; i < iterationsCount; i++)
			{
				stressEvent.Signal();
			}
		}
		int64 endTimeSignal = MT::GetTimeMicroSeconds();


		int64 startTimeWait = MT::GetTimeMicroSeconds();
		{ BROFILER_NEXT_FRAME();
		  BROFILER_CATEGORY("Wait Loop", 0xFFF0F8FF);
			for(int i = 0; i < iterationsCount; i++)
			{
				bool res = stressEvent.Wait(1000);
				CHECK(res == true);
			}
		}
		int64 endTimeWait = MT::GetTimeMicroSeconds();

		double microSecondsPerWait = (double)((endTimeWait - startTimeWait) / (double)iterationsCount);
		double microSecondsPerSignal = (double)((endTimeSignal - startTimeSignal) / (double)iterationsCount);

		printf("microseconds per signal = %3.2f, iterations = %d\n", microSecondsPerSignal, iterationsCount);
		printf("microseconds per wait = %3.2f, iterations = %d\n", microSecondsPerWait, iterationsCount);

#if defined(MT_INSTRUMENTED_BUILD) && defined(MT_ENABLE_BROFILER_SUPPORT)
		{
			printf("Waiting for the 'Brofiler' to be disconnected\n");
			for(;;)
			{
				BROFILER_NEXT_FRAME();
				MT::Thread::Sleep(150);
				if (!Brofiler::IsActive())
				{
					break;
				}
			}
		}

		printf("Done\n");
#endif

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
