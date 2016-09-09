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
#include <MTQueueMPMC.h>
#include <MTConcurrentRingBuffer.h>
#include <MTArrayView.h>
#include <MTStaticVector.h>

SUITE(FoundationTests)
{


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST(RingBufferTest)
{
	MT::ConcurrentRingBuffer<int, 32> ringBuffer;

	ringBuffer.Push(-1);
	ringBuffer.Push(1);

	int tempData[32];
	size_t elementsCount = ringBuffer.PopAll(tempData, MT_ARRAY_SIZE(tempData));
	CHECK_EQUAL(elementsCount, (size_t)2);

	CHECK_EQUAL(tempData[0], -1);
	CHECK_EQUAL(tempData[1], 1);

	int j;
	for(j = 0; j < 507; j++)
	{
		ringBuffer.Push(3 + j);
	}

	elementsCount = ringBuffer.PopAll(tempData, MT_ARRAY_SIZE(tempData));
	CHECK_EQUAL(elementsCount, (size_t)32);

	size_t i;
	for(i = 0; i < elementsCount; i++)
	{
		CHECK_EQUAL(tempData[i], (int)((507+3-32) + i));
	}

}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST(StackArrayTest)
{
	const int elementsCount = 128;

	MT::StaticVector<int, elementsCount> stackArray;

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
TEST(QueueMPMC_BasicTest)
{
	MT::LockFreeQueueMPMC<int, 32> queue;

	for(int i = 0; i < 64; i++)
	{
		bool res = queue.TryPush( std::move(77 + i) );
		if (i < 32)
		{
			CHECK_EQUAL(true, res);
		} else
		{
			CHECK_EQUAL(false, res);
		}
	}

	for(int i = 0; i < 64; i++)
	{
		int val;
		bool res = queue.TryPop(val);
		if (i < 32)
		{
			CHECK_EQUAL(true, res);
			CHECK_EQUAL(77 + i, val);
		} else
		{
			CHECK_EQUAL(false, res);
		}
	}


	CHECK_EQUAL(true, queue.TryPush( 113 ));
	CHECK_EQUAL(true, queue.TryPush( 114 ));
	CHECK_EQUAL(true, queue.TryPush( 115 ));

	int v;
	CHECK_EQUAL(true, queue.TryPop(v));
	CHECK_EQUAL(113, v);

	CHECK_EQUAL(true, queue.TryPush( 116 ));
	CHECK_EQUAL(true, queue.TryPush( 117 ));
	CHECK_EQUAL(true, queue.TryPush( 118 ));

	CHECK_EQUAL(true, queue.TryPop(v));
	CHECK_EQUAL(114, v);
	CHECK_EQUAL(true, queue.TryPop(v));
	CHECK_EQUAL(115, v);
	CHECK_EQUAL(true, queue.TryPop(v));
	CHECK_EQUAL(116, v);
	CHECK_EQUAL(true, queue.TryPop(v));
	CHECK_EQUAL(117, v);
	CHECK_EQUAL(true, queue.TryPop(v));
	CHECK_EQUAL(118, v);

	v = -133;
	CHECK_EQUAL(false, queue.TryPop(v));
	CHECK_EQUAL(-133, v);

}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST(ArrayViewTest)
{

	MT::ArrayView<int> emptyArrayView(nullptr, 0);
	CHECK(emptyArrayView.IsEmpty() == true);

	const int elementsCount = 128;
	void* rawMemory = MT::Memory::Alloc(sizeof(int) * elementsCount);

	MT::ArrayView<int> arrayView(rawMemory, elementsCount);
	CHECK(arrayView.IsEmpty() == false);

	for (int i = 0; i < elementsCount; i++)
	{
		arrayView[i] = (100 + i);
	}
	
	const int* buffer = static_cast<const int*>(rawMemory);
	for (int i = 0; i < elementsCount; i++)
	{
		CHECK_EQUAL(buffer[i], arrayView[i]);
	}

	MT::Memory::Free(rawMemory);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

}
