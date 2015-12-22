#include "Tests.h"
#include <UnitTest++.h>
#include <MTScheduler.h>


#ifdef MT_THREAD_SANITIZER
	#define MT_DEFAULT_WAIT_TIME (50000)
#else
	#define MT_DEFAULT_WAIT_TIME (5000)
#endif

SUITE(SubtasksTests)
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<size_t N>
struct DeepSubtaskQueue
{
	MT_DECLARE_TASK(DeepSubtaskQueue<N>, MT::Color::Blue);

	int result;

	DeepSubtaskQueue() : result(0) {}

	void Do(MT::FiberContext& context)
	{
		DeepSubtaskQueue<N - 1> taskNm1;
		DeepSubtaskQueue<N - 2> taskNm2;

		context.RunSubtasksAndYield(MT::TaskGroup::Default(), &taskNm1, 1);
		context.RunSubtasksAndYield(MT::TaskGroup::Default(), &taskNm2, 1);

		result = taskNm2.result + taskNm1.result;
	}
};

template<>
struct DeepSubtaskQueue<0>
{
	MT_DECLARE_TASK(DeepSubtaskQueue<0>, MT::Color::Blue);

	int result;
	void Do(MT::FiberContext&)
	{
		result = 0;
	}
};


template<>
struct DeepSubtaskQueue<1>
{
	MT_DECLARE_TASK(DeepSubtaskQueue<1>, MT::Color::Blue);

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
		scheduler.RunAsync(MT::TaskGroup::Default(), &task, 1);

		CHECK(scheduler.WaitAll(MT_DEFAULT_WAIT_TIME));

		CHECK_EQUAL(task.result, 144);
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static MT::TaskGroup sourceGroup;
static MT::TaskGroup resultGroup;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct GroupSubtask
{
	MT_DECLARE_TASK(GroupSubtask, MT::Color::Blue);

	void Do(MT::FiberContext& context)
	{
		resultGroup = context.currentGroup;
	}
};

struct GroupTask
{
	MT_DECLARE_TASK(GroupTask, MT::Color::Blue);

	void Do(MT::FiberContext& context)
	{
		GroupSubtask task;
		context.RunSubtasksAndYield(sourceGroup, &task, 1);
	}
};

struct TaskWithManySubtasks
{
	MT_DECLARE_TASK(TaskWithManySubtasks, MT::Color::Blue);

	void Do(MT::FiberContext& context)
	{
		GroupTask task;
		for (int i = 0; i < 2; ++i)
		{
			context.RunSubtasksAndYield(MT::TaskGroup::Default(), &task, 1);
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

	CHECK_EQUAL(sourceGroup.GetValidIndex(), resultGroup.GetValidIndex());
}

// Checks task with multiple subtasks
TEST(OneTaskManySubtasks)
{
	MT::TaskScheduler scheduler;

	sourceGroup = scheduler.CreateGroup();

	TaskWithManySubtasks task;
	scheduler.RunAsync(MT::TaskGroup::Default(), &task, 1);
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
			printf("Timeout: Failed iteration %d\n", i);
			waitAllOK = false;
			break;
		}
	}

	CHECK(waitAllOK);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct TaskSubtaskCombo_Sum1
{
	MT_DECLARE_TASK(TaskSubtaskCombo_Sum1, MT::Color::Blue);

	MT::AtomicInt32* data;

	void Do(MT::FiberContext&)
	{
		data->IncFetch();
	}
};

struct TaskSubtaskCombo_Sum4
{
	MT_DECLARE_TASK(TaskSubtaskCombo_Sum4, MT::Color::Blue);

	MT::AtomicInt32* data;

	TaskSubtaskCombo_Sum1 tasks[2];

	void Do(MT::FiberContext& context)
	{
		tasks[0].data = data;
		tasks[1].data = data;

		context.RunAsync(MT::TaskGroup::Default(), &tasks[0], MT_ARRAY_SIZE(tasks));
		context.RunSubtasksAndYield(MT::TaskGroup::Default(), &tasks[0], MT_ARRAY_SIZE(tasks));
	}
};

struct TaskSubtaskCombo_Sum16
{
	MT_DECLARE_TASK(TaskSubtaskCombo_Sum16, MT::Color::Blue);

	MT::AtomicInt32* data;

	TaskSubtaskCombo_Sum4 tasks[2];

	void Do(MT::FiberContext& context)
	{
		tasks[0].data = data;
		tasks[1].data = data;

		context.RunAsync(MT::TaskGroup::Default(), &tasks[0], MT_ARRAY_SIZE(tasks));
		context.RunSubtasksAndYield(MT::TaskGroup::Default(), &tasks[0], MT_ARRAY_SIZE(tasks));
	}
};

MT::AtomicInt32 sum;

// Checks one simple task
TEST(TaskSubtaskCombo)
{
	sum.Store(0);

	MT::TaskScheduler scheduler;

	TaskSubtaskCombo_Sum16 task[16];
	for (int i = 0; i < 16; ++i)
	{
		task[i].data = &sum;
		scheduler.RunAsync(MT::TaskGroup::Default(), &task[i], 1);
	}

	CHECK(scheduler.WaitAll(MT_DEFAULT_WAIT_TIME));

	CHECK_EQUAL(sum.Load(), 256);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
