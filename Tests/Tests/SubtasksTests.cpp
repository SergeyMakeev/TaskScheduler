#include "Tests.h"
#include <UnitTest++.h>
#include <MTScheduler.h>

#define MT_DEFAULT_WAIT_TIME (5000)

SUITE(SubtasksTests)
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<size_t N>
struct DeepSubtaskQueue : public MT::TaskBase<DeepSubtaskQueue<N>>
{
	DECLARE_DEBUG("DeepSubtaskQueue", MT_COLOR_DEFAULT);

	int result;

	DeepSubtaskQueue() : result(0) {}

	void Do(MT::FiberContext& context)
	{
		DeepSubtaskQueue<N - 1> taskNm1;
		DeepSubtaskQueue<N - 2> taskNm2;

		context.RunSubtasksAndYield(MT::DEFAULT_GROUP, &taskNm1, 1);
		context.RunSubtasksAndYield(MT::DEFAULT_GROUP, &taskNm2, 1);

		result = taskNm2.result + taskNm1.result;
	}
};

template<>
struct DeepSubtaskQueue<0> : public MT::TaskBase<DeepSubtaskQueue<0>>
{
	DECLARE_DEBUG("DeepSubtaskQueue", MT_COLOR_DEFAULT);

	int result;
	void Do(MT::FiberContext&)
	{
		result = 0;
	}
};


template<>
struct DeepSubtaskQueue<1> : public MT::TaskBase<DeepSubtaskQueue<1>>
{
	DECLARE_DEBUG("DeepSubtaskQueue", MT_COLOR_DEFAULT);

	int result;
	void Do(MT::FiberContext&)
	{
		result = 1;
	}
};


// Checks one simple task
TEST(DeepSubtaskQueue)
{
	MT::TaskScheduler scheduler;

	//while(true)
	{
		DeepSubtaskQueue<12> task;
		scheduler.RunAsync(MT::DEFAULT_GROUP, &task, 1);

		CHECK(scheduler.WaitAll(MT_DEFAULT_WAIT_TIME));

		CHECK_EQUAL(task.result, 144);
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static MT::TaskGroup sourceGroup = MT::INVALID_GROUP;
static MT::TaskGroup resultGroup = MT::INVALID_GROUP;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct GroupSubtask : public MT::TaskBase<GroupSubtask>
{
	DECLARE_DEBUG("GroupSubtask", MT_COLOR_DEFAULT);

	void Do(MT::FiberContext& context)
	{
		resultGroup = context.currentGroup;
	}
};

struct GroupTask : public MT::TaskBase<GroupTask>
{
	DECLARE_DEBUG("GroupTask", MT_COLOR_DEFAULT);

	void Do(MT::FiberContext& context)
	{
		GroupSubtask task;
		context.RunSubtasksAndYield(sourceGroup, &task, 1);
	}
};

struct TaskWithManySubtasks : public MT::TaskBase<TaskWithManySubtasks>
{
	DECLARE_DEBUG("TaskWithManySubtasks", MT_COLOR_DEFAULT);

	void Do(MT::FiberContext& context)
	{
		GroupTask task;
		for (int i = 0; i < 2; ++i)
		{
			context.RunSubtasksAndYield(MT::DEFAULT_GROUP, &task, 1);
			MT::Thread::SpinSleep(1);
		}
	}

};

// Checks one simple task
TEST(SubtaskGroup)
{
	MT::TaskScheduler scheduler;

	sourceGroup = scheduler.CreateGroup();

	GroupTask task;
	scheduler.RunAsync(sourceGroup, &task, 1);

	CHECK(scheduler.WaitAll(MT_DEFAULT_WAIT_TIME));

	CHECK_EQUAL(sourceGroup, resultGroup);
}

// Checks task with multiple subtasks
TEST(OneTaskManySubtasks)
{
	MT::TaskScheduler scheduler;

	sourceGroup = scheduler.CreateGroup();

	TaskWithManySubtasks task;
	scheduler.RunAsync(MT::DEFAULT_GROUP, &task, 1);
	CHECK(scheduler.WaitAll(MT_DEFAULT_WAIT_TIME));
}

// Checks many simple task with subtasks
TEST(ManyTasksOneSubtask)
{
	MT::TaskScheduler scheduler;

	bool waitAllOK = true;

	sourceGroup = scheduler.CreateGroup();

	for (int i = 0; i < 100000; ++i)
	{
		GroupTask group;
		scheduler.RunAsync(sourceGroup, &group, 1);
		//if (!scheduler.WaitAll(MT_DEFAULT_WAIT_TIME))
		if (!scheduler.WaitGroup(sourceGroup, MT_DEFAULT_WAIT_TIME))
		{
			printf("Failed iteration %d\n", i);
			waitAllOK = false;
			break;
		}
	}

	CHECK(waitAllOK);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct TaskSubtaskCombo_Sum1 : public MT::TaskBase<TaskSubtaskCombo_Sum1>
{
	DECLARE_DEBUG("TaskSubtaskCombo_Sum1", MT_COLOR_DEFAULT);

	MT::AtomicInt* data;

	void Do(MT::FiberContext&)
	{
		data->Inc();
	}
};

struct TaskSubtaskCombo_Sum4 : public MT::TaskBase<TaskSubtaskCombo_Sum4>
{
	DECLARE_DEBUG("TaskSubtaskCombo_Sum4", MT_COLOR_DEFAULT);

	MT::AtomicInt* data;

	TaskSubtaskCombo_Sum1 tasks[2];

	void Do(MT::FiberContext& context)
	{
		tasks[0].data = data;
		tasks[1].data = data;

		context.RunAsync(MT::DEFAULT_GROUP, &tasks[0], MT_ARRAY_SIZE(tasks));
		context.RunSubtasksAndYield(MT::DEFAULT_GROUP, &tasks[0], MT_ARRAY_SIZE(tasks));
	}
};

struct TaskSubtaskCombo_Sum16 : public MT::TaskBase<TaskSubtaskCombo_Sum16>
{
	DECLARE_DEBUG("TaskSubtaskCombo_Sum16", MT_COLOR_DEFAULT);

	MT::AtomicInt* data;

	TaskSubtaskCombo_Sum4 tasks[2];

	void Do(MT::FiberContext& context)
	{
		tasks[0].data = data;
		tasks[1].data = data;

		context.RunAsync(MT::DEFAULT_GROUP, &tasks[0], MT_ARRAY_SIZE(tasks));
		context.RunSubtasksAndYield(MT::DEFAULT_GROUP, &tasks[0], MT_ARRAY_SIZE(tasks));
	}
};

MT::AtomicInt sum;

// Checks one simple task
TEST(TaskSubtaskCombo)
{
	sum.Set(0);

	MT::TaskScheduler scheduler;

	TaskSubtaskCombo_Sum16 task[16];
	for (int i = 0; i < 16; ++i)
	{
		task[i].data = &sum;
		scheduler.RunAsync(MT::DEFAULT_GROUP, &task[i], 1);
	}

	CHECK(scheduler.WaitAll(MT_DEFAULT_WAIT_TIME));

	CHECK_EQUAL(sum.Get(), 256);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
