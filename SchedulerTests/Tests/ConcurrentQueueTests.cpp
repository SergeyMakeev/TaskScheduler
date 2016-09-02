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


SUITE(ConcurrentQueueTests)
{

/*
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	TEST(BasicQueueTest)
	{
		MT::ConcurrentQueueLIFO<int> queue;

		const int elementsCount = 3 + MT::ConcurrentQueueLIFO<int>::CAPACITY * 3;
		CHECK(elementsCount > MT::ConcurrentQueueLIFO<int>::CAPACITY);


		queue.Push(0);
		for(int i = 1; i < elementsCount; i++)
		{
			queue.Push(i);

			int v = -1;
			bool res = queue.TryPopFront(v);
			CHECK_EQUAL(true, res);
			CHECK_EQUAL(i-1, v);
		}

		int results[ MT::ConcurrentQueueLIFO<int>::CAPACITY ];
		size_t count = queue.PopAll(&results[0], elementsCount);

		CHECK_EQUAL(count, (size_t)1 );

		CHECK_EQUAL( (elementsCount - 1), results[0] );
	}
*/




/*
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	TEST(BasicQueueTest2)
	{
		MT::ConcurrentQueueLIFO<int> queue;

		const int elementsCount = 11 + MT::ConcurrentQueueLIFO<int>::CAPACITY * 3;
		CHECK(elementsCount > MT::ConcurrentQueueLIFO<int>::CAPACITY);


		const int primeNumber = 13;
		int buffer[primeNumber];

		for(uint32 j = 0; j < MT_ARRAY_SIZE(buffer); j++)
		{
			buffer[j] = 0;
		}
		queue.PushRange(buffer, MT_ARRAY_SIZE(buffer));
		for(int i = 1; i < elementsCount; i++)
		{
			//fill buffer
			for(uint32 j = 0; j < MT_ARRAY_SIZE(buffer); j++)
			{
				buffer[j] = i;
			}
			//push buffer to queue
			queue.PushRange(buffer, MT_ARRAY_SIZE(buffer));

			//pop from queue
			for(uint32 j = 0; j < MT_ARRAY_SIZE(buffer); j++)
			{
				int v = -1;
				bool res = queue.TryPopFront(v);
				CHECK_EQUAL(true, res);
				CHECK_EQUAL(i-1, v);
			}
		}

		int results[ MT::ConcurrentQueueLIFO<int>::CAPACITY ];
		size_t count = queue.PopAll(&results[0], elementsCount);

		CHECK_EQUAL(count, (size_t)primeNumber );
		CHECK_EQUAL(count, MT_ARRAY_SIZE(buffer) );

		int vv = -1;
		bool res = queue.TryPopFront( vv );
		CHECK_EQUAL(false, res);
		CHECK_EQUAL(-1, vv);

		res = queue.TryPopBack( vv );
		CHECK_EQUAL(false, res);
		CHECK_EQUAL(-1, vv);

		for(uint32 j = 0; j < MT_ARRAY_SIZE(buffer); j++)
		{
			CHECK_EQUAL( (elementsCount - 1), results[j] );
		}
		
	}
*/


	TEST(QueueTest)
	{
		MT::ConcurrentQueueLIFO<int> lifoQueue;

		lifoQueue.Push(1);
		lifoQueue.Push(3);
		lifoQueue.Push(7);
		lifoQueue.Push(10);
		lifoQueue.Push(13);

		int res = 0;

		CHECK(lifoQueue.TryPopBack(res) == true);
		CHECK_EQUAL(res, 13);

		CHECK(lifoQueue.TryPopBack(res) == true);
		CHECK_EQUAL(res, 10);

		CHECK(lifoQueue.TryPopBack(res) == true);
		CHECK_EQUAL(res, 7);

		CHECK(lifoQueue.TryPopBack(res) == true);
		CHECK_EQUAL(res, 3);

		CHECK(lifoQueue.TryPopBack(res) == true);
		CHECK_EQUAL(res, 1);

		CHECK(lifoQueue.TryPopBack(res) == false);

		lifoQueue.Push(4);

		CHECK(lifoQueue.TryPopBack(res) == true);
		CHECK_EQUAL(res, 4);

		CHECK(lifoQueue.TryPopBack(res) == false);

		CHECK(lifoQueue.IsEmpty() == true);

		lifoQueue.Push(101);
		lifoQueue.Push(103);
		lifoQueue.Push(107);
		lifoQueue.Push(1010);
		lifoQueue.Push(1013);

		CHECK(lifoQueue.IsEmpty() == false);

		//CHECK(lifoQueue.TryPopFront(res) == true);
		//CHECK_EQUAL(res, 101);

		//CHECK(lifoQueue.TryPopFront(res) == true);
		//CHECK_EQUAL(res, 103);

		int tempData[16];
		size_t elementsCount = lifoQueue.PopAll(tempData, MT_ARRAY_SIZE(tempData));
		CHECK_EQUAL(elementsCount, (size_t)5);

		CHECK_EQUAL(tempData[0], 101);
		CHECK_EQUAL(tempData[1], 103);
		CHECK_EQUAL(tempData[2], 107);
		CHECK_EQUAL(tempData[3], 1010);
		CHECK_EQUAL(tempData[4], 1013);
	}
}
