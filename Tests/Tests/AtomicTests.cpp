#include "Tests.h"
#include <UnitTest++.h>
#include <MTScheduler.h>


SUITE(CleanupTests)
{
	static const int OLD_VALUE = 1;
	static const int VALUE = 13;
	static const int NEW_VALUE = 16;
	static const int RELAXED_VALUE = 27;

TEST(AtomicSimpleTest)
{
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
