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
#include <math.h>
#include <UnitTest++.h>
#include <MTAtomic.h>
#include <MTScheduler.h>


SUITE(AtomicTests)
{
	static const int OLD_VALUE = 1;
	static const int VALUE = 13;
	static const int NEW_VALUE = 16;
	static const int RELAXED_VALUE = 27;

	void TestStatics()
	{
		// This variables must be placed to .data / .bss section
		//
		// From "Cpp Standard"
		//
		// 6.7 Declaration statement
		//
		// 4 The zero-initialization (8.5) of all block-scope variables with static storage duration (3.7.1) or thread storage
		//   duration (3.7.2) is performed before any other initialization takes place.
		//   Constant initialization (3.6.2) of a block-scope entity with static storage duration, if applicable,
		//   is performed before its block is first entered.
		//
		static MT::Atomic32Base<int32> test = { 0 };
		static MT::AtomicPtrBase<void> pTest = { nullptr };

		test.Store(13);
		pTest.Store(nullptr);

		CHECK_EQUAL(13, test.Load());
		CHECK(pTest.Load() == nullptr);
	}

TEST(AtomicSimpleTest)
{
	TestStatics();

	MT::Atomic32<int32> test_relaxed;
	test_relaxed.StoreRelaxed(RELAXED_VALUE);
	CHECK(test_relaxed.Load() == RELAXED_VALUE);

	MT::Atomic32<int32> test;
	test.Store(OLD_VALUE);
	CHECK(test.Load() == OLD_VALUE);

	int prevValue = test.Exchange(VALUE);
	CHECK(test.Load() == VALUE);
	CHECK(prevValue == OLD_VALUE);

	int nowValue = test.IncFetch();
	CHECK(nowValue == (VALUE+1));

	nowValue = test.DecFetch();
	CHECK(nowValue == VALUE);

	nowValue = test.AddFetch(VALUE);
	CHECK(nowValue == (VALUE+VALUE));

	MT::Atomic32<int32> test2(VALUE);
	CHECK(test2.Load() == VALUE);

	int prevResult = test2.CompareAndSwap(NEW_VALUE, OLD_VALUE);
	CHECK(prevResult == VALUE);
	CHECK(test2.Load() == VALUE);

	prevResult = test2.CompareAndSwap(VALUE, NEW_VALUE);
	CHECK(prevResult == VALUE);
	CHECK(test2.Load() == NEW_VALUE);

	MT::Atomic32<uint32> test3(UINT32_MAX);
	CHECK_EQUAL(UINT32_MAX, test3.Load());

	//check for wraps
	uint32 uNowValue = test3.IncFetch();
	CHECK(uNowValue == 0);

	uNowValue = test3.DecFetch();
	CHECK(uNowValue == UINT32_MAX);


	char tempObject;
	char* testPtr = &tempObject;
	char* testPtrNew = testPtr + 1;


	MT::AtomicPtr<char> atomicPtrRelaxed;
	atomicPtrRelaxed.StoreRelaxed(testPtr);
	CHECK(atomicPtrRelaxed.Load() == testPtr);

	MT::AtomicPtr<char> atomicPtr;
	CHECK(atomicPtr.Load() == nullptr);

	atomicPtr.Store(testPtr);
	CHECK(atomicPtr.Load() == testPtr);

	char* prevPtr = atomicPtr.CompareAndSwap(nullptr, testPtrNew);
	CHECK(prevPtr == testPtr);
	CHECK(atomicPtr.Load() == testPtr);

	prevPtr = atomicPtr.CompareAndSwap(testPtr, testPtrNew);
	CHECK(prevPtr == testPtr);
	CHECK(atomicPtr.Load() == testPtrNew);

	char* prevPtr2 = atomicPtr.Exchange(nullptr);
	CHECK(prevPtr2 == testPtrNew);
	CHECK(atomicPtr.Load() == nullptr);
}


MT::Atomic32<uint32> isReady;
MT::Atomic32<uint32> a;
MT::Atomic32<uint32> b;

uint32 sharedValue = 0;

MT::Atomic32<uint32> simpleLock;

void ThreadFunc( void* userData )
{
	MT_UNUSED(userData);

	MT::SpinWait spinWait;

	while(isReady.LoadRelaxed() == 0)
	{
		spinWait.SpinOnce();
	}

	for(int iteration = 0; iteration < 10000000; iteration++)
	{
		uint32 prevA = a.AddFetch(1);
		uint32 prevB = b.AddFetch(1);

		// A should be less than B, but can also be a equal due to threads race 
		CHECK(prevA <= prevB);
		if (prevA > prevB)
		{
			printf("a = %d, b = %d\n", prevA, prevB);
			break;
		}
	}

	float res = 0.0f;
	uint32 randDelay = 1 + (rand() % 4);
	uint32 count = 0;
	while (count < 10000000)
    {
		res = 0.0f;
		for(uint32 i = 0; i < randDelay; i++)
		{
			res += sin((float)i);
		}

		if (simpleLock.CompareAndSwap(0, 1) == 0)
		{
			sharedValue++;
			simpleLock.Store(0);
			count++;
		}
	}

	//prevent compiler optimization
	printf("%3.2f\n", res);

	//
}

/*

Inspired by "This Is Why They Call It a Weakly-Ordered CPU" blog post by Jeff Preshing
http://preshing.com/20121019/this-is-why-they-call-it-a-weakly-ordered-cpu/

*/
TEST(AtomicOrderingTest)
{
	isReady.Store(0);

	a.Store(1);
	b.Store(2);

	sharedValue = 0;

	simpleLock.Store(0);


	MT::Thread threads[2];
	uint32 threadsCount = MT_ARRAY_SIZE(threads);

	printf("threads count %d\n", threadsCount);

	for(uint32 i = 0; i < threadsCount; i++)
	{
		threads[i].Start(16384, ThreadFunc, nullptr);
	}

	isReady.Store(1);

	for(uint32 i = 0; i < threadsCount; i++)
	{
		threads[i].Join();
	}

	uint32 expectedSharedValue = (10000000 * threadsCount);
	CHECK_EQUAL(sharedValue, expectedSharedValue);
	
	
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
