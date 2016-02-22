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
#include <MTAtomic.h>


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
		static MT::AtomicInt32Base test = { 0 };
		static MT::AtomicPtrBase pTest = { nullptr };

		test.Store(13);
		pTest.Store(nullptr);

		CHECK_EQUAL(13, test.Load());
		CHECK(pTest.Load() == nullptr);
	}

TEST(AtomicSimpleTest)
{
	TestStatics();

	MT::AtomicInt32 test_relaxed;
	test_relaxed.StoreRelaxed(RELAXED_VALUE);
	CHECK(test_relaxed.Load() == RELAXED_VALUE);

	MT::AtomicInt32 test;
	test.Store(OLD_VALUE);
	CHECK(test.Load() == OLD_VALUE);

	int prevValue = test.Store(VALUE);
	CHECK(test.Load() == VALUE);
	CHECK(prevValue == OLD_VALUE);

	int nowValue = test.IncFetch();
	CHECK(nowValue == (VALUE+1));

	nowValue = test.DecFetch();
	CHECK(nowValue == VALUE);

	nowValue = test.AddFetch(VALUE);
	CHECK(nowValue == (VALUE+VALUE));

	MT::AtomicInt32 test2(VALUE);
	CHECK(test2.Load() == VALUE);

	int prevResult = test2.CompareAndSwap(NEW_VALUE, OLD_VALUE);
	CHECK(prevResult == VALUE);
	CHECK(test2.Load() == VALUE);

	prevResult = test2.CompareAndSwap(VALUE, NEW_VALUE);
	CHECK(prevResult == VALUE);
	CHECK(test2.Load() == NEW_VALUE);


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



}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
