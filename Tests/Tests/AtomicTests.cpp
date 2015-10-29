#include "Tests.h"
#include <UnitTest++.h>
#include <MTScheduler.h>


SUITE(CleanupTests)
{
	static const int OLD_VALUE = 1;
	static const int VALUE = 13;
	static const int NEW_VALUE = 16;

TEST(AtomicSimpleTest)
{
	MT::AtomicInt test;
	test.Set(OLD_VALUE);
	CHECK(test.Get() == OLD_VALUE);

	int prevValue = test.Set(VALUE);
	CHECK(test.Get() == VALUE);
	CHECK(prevValue == OLD_VALUE);

	int nowValue = test.Inc();
	CHECK(nowValue == (VALUE+1));

	nowValue = test.Dec();
	CHECK(nowValue == VALUE);

	nowValue = test.Add(VALUE);
	CHECK(nowValue == (VALUE+VALUE));

	MT::AtomicInt test2(VALUE);
	CHECK(test2.Get() == VALUE);

	int prevResult = test2.CompareAndSwap(NEW_VALUE, OLD_VALUE);
	CHECK(prevResult == VALUE);
	CHECK(test2.Get() == VALUE);

	prevResult = test2.CompareAndSwap(VALUE, NEW_VALUE);
	CHECK(prevResult == VALUE);
	CHECK(test2.Get() == NEW_VALUE);


	char tempObject;
	char* testPtr = &tempObject;
	char* testPtrNew = testPtr + 1;

	MT::AtomicPtr<char> atomicPtr;
	CHECK(atomicPtr.Get() == nullptr);

	atomicPtr.Set(testPtr);
	CHECK(atomicPtr.Get() == testPtr);

	char* prevPtr = atomicPtr.CompareAndSwap(nullptr, testPtrNew);
	CHECK(prevPtr == testPtr);
	CHECK(atomicPtr.Get() == testPtr);

	prevPtr = atomicPtr.CompareAndSwap(testPtr, testPtrNew);
	CHECK(prevPtr == testPtr);
	CHECK(atomicPtr.Get() == testPtrNew);



}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
