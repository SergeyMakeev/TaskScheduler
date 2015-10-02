#include "Tests.h"
#include <UnitTest++.h>
#include <MTScheduler.h>



SUITE(FireAndForget)
{


struct SimpleTask : public MT::TaskBase<SimpleTask>
{
	MT_DECLARE_DEBUG_INFO("SimpleTask", MT_COLOR_DEFAULT);


	SimpleTask()
	{
	}

	SimpleTask(SimpleTask&&)
	{
	}

	void Do(MT::FiberContext&)
	{
	}
};



TEST(PoolTest)
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
	MT::TaskHandle taskHandle4 = taskPool.Alloc(SimpleTask());
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


/*
// 
TEST(FireAndForgetSimple)
{
}
*/
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
