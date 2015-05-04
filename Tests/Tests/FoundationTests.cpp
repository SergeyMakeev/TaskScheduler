#include "Tests.h"
#include <UnitTest++.h>
#include <MTScheduler.h>
#include <MTConcurrentQueueLIFO.h>
#include <MTConcurrentRingBuffer.h>
#include <MTWrapperArray.h>
#include <MTStackArray.h>

SUITE(FoundationTests)
{

TEST(QueueTest)
{
	MT::ConcurrentQueueLIFO<int> lifoQueue;

	lifoQueue.Push(1);
	lifoQueue.Push(3);
	lifoQueue.Push(7);
	lifoQueue.Push(10);
	lifoQueue.Push(13);

	int res = 0;

	CHECK(lifoQueue.TryPop(res) == true);
	CHECK_EQUAL(res, 13);

	CHECK(lifoQueue.TryPop(res) == true);
	CHECK_EQUAL(res, 10);

	CHECK(lifoQueue.TryPop(res) == true);
	CHECK_EQUAL(res, 7);

	CHECK(lifoQueue.TryPop(res) == true);
	CHECK_EQUAL(res, 3);

	CHECK(lifoQueue.TryPop(res) == true);
	CHECK_EQUAL(res, 1);

	CHECK(lifoQueue.TryPop(res) == false);

	lifoQueue.Push(4);

	CHECK(lifoQueue.TryPop(res) == true);
	CHECK_EQUAL(res, 4);

	CHECK(lifoQueue.TryPop(res) == false);

	CHECK(lifoQueue.IsEmpty() == true);

	lifoQueue.Push(101);
	lifoQueue.Push(103);
	lifoQueue.Push(107);
	lifoQueue.Push(1010);
	lifoQueue.Push(1013);

	CHECK(lifoQueue.IsEmpty() == false);

	int tempData[16];
	size_t elementsCount = lifoQueue.PopAll(tempData, ARRAY_SIZE(tempData));
	CHECK_EQUAL(elementsCount, (size_t)5);

	CHECK_EQUAL(tempData[0], 101);
	CHECK_EQUAL(tempData[1], 103);
	CHECK_EQUAL(tempData[2], 107);
	CHECK_EQUAL(tempData[3], 1010);
	CHECK_EQUAL(tempData[4], 1013);

}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST(RingBufferTest)
{
	MT::ConcurrentRingBuffer<int, 32> ringBuffer;

	ringBuffer.Push(-1);
	ringBuffer.Push(1);

	int tempData[32];
	size_t elementsCount = ringBuffer.PopAll(tempData, ARRAY_SIZE(tempData));
	CHECK_EQUAL(elementsCount, (size_t)2);

	CHECK_EQUAL(tempData[0], -1);
	CHECK_EQUAL(tempData[1], 1);

	size_t i;
	for(i = 0; i < 507; i++)
	{
		ringBuffer.Push(3 + i);
	}

	elementsCount = ringBuffer.PopAll(tempData, ARRAY_SIZE(tempData));
	CHECK_EQUAL(elementsCount, (size_t)32);

	for(i = 0; i < elementsCount; i++)
	{
		CHECK_EQUAL(tempData[i], (int)((507+3-32) + i));
	}

}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST(StackArrayTest)
{
	const int elementsCount = 128;

	MT::StackArray<int, elementsCount> stackArray;

	CHECK(stackArray.IsEmpty() == true);

	stackArray.PushBack(200);
	CHECK(stackArray.IsEmpty() == false);
	CHECK_EQUAL(stackArray.Size(), (size_t)1);

	for(int i = 1; i < elementsCount; i++)
	{
		stackArray.PushBack(200 + i);
	}

	CHECK(stackArray.IsEmpty() == false);
	CHECK_EQUAL(stackArray.Size(), (size_t)elementsCount);

	for(int i = 0; i < elementsCount; i++)
	{
		CHECK_EQUAL(stackArray[i], (200 + i));
	}

}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST(WrapperArrayTest)
{

	MT::WrapperArray<int> emptyWrapperArray(nullptr, 0);
	CHECK(emptyWrapperArray.IsEmpty() == true);

	const int elementsCount = 128;
	void * rawMemory = malloc(sizeof(int) * elementsCount);

	MT::WrapperArray<int> wrapperArray(rawMemory, elementsCount);
	CHECK(wrapperArray.IsEmpty() == false);

	for (int i = 0; i < elementsCount; i++)
	{
		wrapperArray[i] = (100 + i);
	}
	
	const int* buffer = static_cast<const int*>(rawMemory);
	for (int i = 0; i < elementsCount; i++)
	{
		CHECK_EQUAL(buffer[i], wrapperArray[i]);
	}

	free(rawMemory);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

}
