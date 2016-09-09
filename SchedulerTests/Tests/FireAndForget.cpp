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



SUITE(FireAndForget)
{

struct SimpleTask;

typedef MT::TaskPool<SimpleTask, 512> TestPoolType;

struct SimpleTask
{
	MT_DECLARE_TASK(SimpleTask, MT::StackRequirements::STANDARD, MT::TaskPriority::NORMAL, MT::Color::Blue);

	MT::Atomic32<int32>* doCounter;
	MT::Atomic32<int32>* dtorCounter;
	TestPoolType* taskPool;

	SimpleTask()
		: doCounter(nullptr)
		, dtorCounter(nullptr)
		, taskPool(nullptr)
	{
	}

	SimpleTask(MT::Atomic32<int32>* _doCounter, MT::Atomic32<int32>* _dtorCounter, TestPoolType * _taskPool)
		: doCounter(_doCounter)
		, dtorCounter(_dtorCounter)
		, taskPool(_taskPool)
	{
	}

	SimpleTask(SimpleTask&& other)
		: doCounter(other.doCounter)
		, dtorCounter(other.dtorCounter)
		, taskPool(other.taskPool)
	{
		other.doCounter = nullptr;
		other.dtorCounter = nullptr;
		other.taskPool = nullptr;
	}

	~SimpleTask()
	{
		if (dtorCounter)
		{
			dtorCounter->IncFetch();
		}
	}

	void Do(MT::FiberContext& context)
	{
		if (doCounter)
		{
			doCounter->IncFetch();
		}

		if (taskPool)
		{
			MT::TaskHandle handle = taskPool->Alloc(SimpleTask(doCounter, dtorCounter, nullptr));

			context.RunSubtasksAndYield(MT::TaskGroup::Default(), &handle, 1);
		}
	}
};



TEST(SingleThreadPoolTest)
{
	MT::TaskPool<SimpleTask, 4> taskPool;

	MT::TaskHandle taskHandle0 = taskPool.Alloc(SimpleTask());
	CHECK_EQUAL(true, taskHandle0.IsValid());

	MT::TaskHandle taskHandle1 = taskPool.Alloc(SimpleTask());
	CHECK_EQUAL(true, taskHandle1.IsValid());

	MT::TaskHandle taskHandle2 = taskPool.Alloc(SimpleTask());
	CHECK_EQUAL(true, taskHandle2.IsValid());

	MT::TaskHandle taskHandle3 = taskPool.Alloc(SimpleTask());
	CHECK_EQUAL(true, taskHandle3.IsValid());

	CHECK_EQUAL(true, taskHandle0.IsValid());
	CHECK_EQUAL(true, taskHandle1.IsValid());
	CHECK_EQUAL(true, taskHandle2.IsValid());
	CHECK_EQUAL(true, taskHandle3.IsValid());


	// check for allocation fail
	MT::TaskHandle taskHandle4 = taskPool.TryAlloc(SimpleTask());
	CHECK_EQUAL(false, taskHandle4.IsValid());


	// check state
	CHECK_EQUAL(true, taskHandle0.IsValid());
	CHECK_EQUAL(true, taskHandle1.IsValid());
	CHECK_EQUAL(true, taskHandle2.IsValid());
	CHECK_EQUAL(true, taskHandle3.IsValid());
	CHECK_EQUAL(false, taskHandle4.IsValid());

	// destroy pool task by handle
	CHECK_EQUAL(true, MT::PoolElementHeader::DestoryByHandle(taskHandle0));
	CHECK_EQUAL(true, MT::PoolElementHeader::DestoryByHandle(taskHandle1));
	CHECK_EQUAL(true, MT::PoolElementHeader::DestoryByHandle(taskHandle2));
	CHECK_EQUAL(true, MT::PoolElementHeader::DestoryByHandle(taskHandle3));
	CHECK_EQUAL(false, MT::PoolElementHeader::DestoryByHandle(taskHandle4));

	// check for double destroy
	CHECK_EQUAL(false, MT::PoolElementHeader::DestoryByHandle(taskHandle0));
	CHECK_EQUAL(false, MT::PoolElementHeader::DestoryByHandle(taskHandle3));

	MT::TaskHandle taskHandle5 = taskPool.Alloc(SimpleTask());

	CHECK_EQUAL(false, taskHandle0.IsValid());
	CHECK_EQUAL(false, taskHandle1.IsValid());
	CHECK_EQUAL(false, taskHandle2.IsValid());
	CHECK_EQUAL(false, taskHandle3.IsValid());
	CHECK_EQUAL(false, taskHandle4.IsValid());
	CHECK_EQUAL(true, taskHandle5.IsValid());
}


struct ThreadTest
{
	MT_DECLARE_TASK(ThreadTest, MT::StackRequirements::STANDARD, MT::TaskPriority::NORMAL, MT::Color::Blue);

	TestPoolType * taskPool;

	void Do(MT::FiberContext&)
	{
		for (int i = 0; i < 20000; i++)
		{
			MT::TaskHandle handle = taskPool->TryAlloc(SimpleTask());
			if (handle.IsValid())
			{
				CHECK_EQUAL(true, MT::PoolElementHeader::DestoryByHandle(handle));
			} else
			{
				CHECK_EQUAL(false, MT::PoolElementHeader::DestoryByHandle(handle));
			}
		}
	}
};


TEST(MultiThreadPoolTest)
{
	TestPoolType taskPool;

	MT::TaskScheduler scheduler;

	ThreadTest tasks[8];
	for (size_t i = 0; i < MT_ARRAY_SIZE(tasks); ++i)
	{
		tasks[i].taskPool = &taskPool;
	}

	scheduler.RunAsync(MT::TaskGroup::Default(), &tasks[0], MT_ARRAY_SIZE(tasks));

	int timeout = 20000;
	CHECK(scheduler.WaitGroup(MT::TaskGroup::Default(), timeout));
}


//
TEST(FireAndForgetSimple)
{
	MT::Atomic32<int32> doCounter(0);
	MT::Atomic32<int32> dtorCounter(0);

	MT::TaskScheduler scheduler;
	TestPoolType taskPool;

	for(int pass = 0; pass < 4; pass++)
	{
		printf("--- step %d ---\n", pass);

		doCounter.Store(0);
		dtorCounter.Store(0);

		MT::TaskHandle taskHandles[250];
		for (size_t i = 0; i < MT_ARRAY_SIZE(taskHandles); ++i)
		{
			taskHandles[i] = taskPool.Alloc(SimpleTask(&doCounter, &dtorCounter, &taskPool));
			CHECK_EQUAL(true, taskHandles[i].IsValid());
		}

		scheduler.RunAsync(MT::TaskGroup::Default(), &taskHandles[0], MT_ARRAY_SIZE(taskHandles));

		int timeout = 20000;
		CHECK(scheduler.WaitAll(timeout));

		CHECK_EQUAL(MT_ARRAY_SIZE(taskHandles) * 2, (size_t)doCounter.Load());
		CHECK_EQUAL(MT_ARRAY_SIZE(taskHandles) * 2, (size_t)dtorCounter.Load());
	}

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
