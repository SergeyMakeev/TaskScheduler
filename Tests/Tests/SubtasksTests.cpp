#include "Tests.h"
#include <UnitTest++.h>
#include <MTScheduler.h>

SUITE(SubtasksTests)
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<size_t N>
struct DeepSubtaskQueue : public MT::TaskBase<DeepSubtaskQueue<N>>
{
	DECLARE_DEBUG("DeepSubtaskQueue", DEFAULT_COLOR);

	int result;

	DeepSubtaskQueue() : result(0) {}

	void Do(MT::FiberContext& context)
	{
		DeepSubtaskQueue<N - 1> taskNm1;
		DeepSubtaskQueue<N - 2> taskNm2;

		context.RunSubtasksAndYield(MT::TaskGroup::GROUP_0, &taskNm1, 1);
		context.RunSubtasksAndYield(MT::TaskGroup::GROUP_0, &taskNm2, 1);

		result = taskNm2.result + taskNm1.result;
	}
};

template<>
struct DeepSubtaskQueue<0> : public MT::TaskBase<DeepSubtaskQueue<0>>
{
	DECLARE_DEBUG("DeepSubtaskQueue", DEFAULT_COLOR);

	int result;
	void Do(MT::FiberContext&)
	{
		result = 0;
	}
};


template<>
struct DeepSubtaskQueue<1> : public MT::TaskBase<DeepSubtaskQueue<1>>
{
	DECLARE_DEBUG("DeepSubtaskQueue", DEFAULT_COLOR);

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
		scheduler.RunAsync(MT::TaskGroup::GROUP_0, &task, 1);

		CHECK(scheduler.WaitAll(200));

		CHECK_EQUAL(task.result, 144);
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static MT::TaskGroup::Type sourceGroup = MT::TaskGroup::GROUP_1;
static MT::TaskGroup::Type resultGroup = MT::TaskGroup::GROUP_UNDEFINED;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct GroupSubtask : public MT::TaskBase<GroupSubtask>
{
	DECLARE_DEBUG("GroupSubtask", DEFAULT_COLOR);

	void Do(MT::FiberContext& context)
	{
		resultGroup = context.currentGroup;
	}
};

struct GroupTask : public MT::TaskBase<GroupTask>
{
	DECLARE_DEBUG("GroupTask", DEFAULT_COLOR);

	void Do(MT::FiberContext& context)
	{
		GroupSubtask task;
		context.RunSubtasksAndYield(sourceGroup, &task, 1);
	}
};

struct TaskWithManySubtasks : public MT::TaskBase<TaskWithManySubtasks>
{
	DECLARE_DEBUG("TaskWithManySubtasks", DEFAULT_COLOR);

	void Do(MT::FiberContext& context)
	{
		GroupTask task;
		for (int i = 0; i < 2; ++i)
		{
			context.RunSubtasksAndYield(MT::TaskGroup::GROUP_0, &task, 1);
			MT::Thread::SpinSleep(1);
		}
	}

};

// Checks one simple task
TEST(SubtaskGroup)
{
	MT::TaskScheduler scheduler;

	GroupTask task;
	scheduler.RunAsync(sourceGroup, &task, 1);

	CHECK(scheduler.WaitAll(200));

	CHECK_EQUAL(sourceGroup, resultGroup);
}

// Checks task with multiple subtasks
TEST(OneTaskManySubtasks)
{
	MT::TaskScheduler scheduler;
	TaskWithManySubtasks task;
	scheduler.RunAsync(sourceGroup, &task, 1);
	CHECK(scheduler.WaitAll(100));
}

// Checks many simple task with subtasks
TEST(ManyTasksOneSubtask)
{
	MT::TaskScheduler scheduler;

	bool waitAllOK = true;

	for (int i = 0; i < 100000 && waitAllOK; ++i)
	{
		GroupSubtask group;
		scheduler.RunAsync(sourceGroup, &group, 1);
		waitAllOK = waitAllOK && scheduler.WaitAll(200);
	}

	CHECK(waitAllOK);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct TaskSubtaskCombo_Sum1 : public MT::TaskBase<TaskSubtaskCombo_Sum1>
{
	DECLARE_DEBUG("TaskSubtaskCombo_Sum1", DEFAULT_COLOR);

	MT::AtomicInt* data;

	void Do(MT::FiberContext&)
	{
		data->Inc();
	}
};

struct TaskSubtaskCombo_Sum4 : public MT::TaskBase<TaskSubtaskCombo_Sum4>
{
	DECLARE_DEBUG("TaskSubtaskCombo_Sum4", DEFAULT_COLOR);

	MT::AtomicInt* data;

	TaskSubtaskCombo_Sum1 tasks[2];

	void Do(MT::FiberContext& context)
	{
		tasks[0].data = data;
		tasks[1].data = data;

		context.RunAsync(MT::TaskGroup::GROUP_2, &tasks[0], ARRAY_SIZE(tasks));
		context.RunSubtasksAndYield(MT::TaskGroup::GROUP_0, &tasks[0], ARRAY_SIZE(tasks));
	}
};

struct TaskSubtaskCombo_Sum16 : public MT::TaskBase<TaskSubtaskCombo_Sum16>
{
	DECLARE_DEBUG("TaskSubtaskCombo_Sum16", DEFAULT_COLOR);

	MT::AtomicInt* data;

	TaskSubtaskCombo_Sum4 tasks[2];

	void Do(MT::FiberContext& context)
	{
		tasks[0].data = data;
		tasks[1].data = data;

		context.RunAsync(MT::TaskGroup::GROUP_1, &tasks[0], ARRAY_SIZE(tasks));
		context.RunSubtasksAndYield(MT::TaskGroup::GROUP_0, &tasks[0], ARRAY_SIZE(tasks));
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
		scheduler.RunAsync(MT::TaskGroup::GROUP_0, &task[i], 1);
	}

	CHECK(scheduler.WaitAll(200));

	CHECK_EQUAL(sum.Get(), 256);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
