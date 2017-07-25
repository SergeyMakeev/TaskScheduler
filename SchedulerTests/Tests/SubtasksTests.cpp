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

#include "../Profiler/Profiler.h"


#ifdef MT_THREAD_SANITIZER
	#define MT_DEFAULT_WAIT_TIME (500000)
	#define MT_SUBTASK_QUEUE_DEEP (3)
	#define MT_ITERATIONS_COUNT (10)
#else
	#define MT_DEFAULT_WAIT_TIME (5000)
	#define MT_SUBTASK_QUEUE_DEEP (12)
	#define MT_ITERATIONS_COUNT (100000)
#endif

SUITE(SubtasksTests)
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<size_t N>
struct DeepSubtaskQueue
{
	MT_DECLARE_TASK(DeepSubtaskQueue<N>, MT::StackRequirements::STANDARD, MT::TaskPriority::NORMAL, MT::Color::Blue);

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
	MT_DECLARE_TASK(DeepSubtaskQueue<0>, MT::StackRequirements::STANDARD, MT::TaskPriority::NORMAL, MT::Color::Blue);

	int result;
	void Do(MT::FiberContext&)
	{
		result = 0;
	}
};


template<>
struct DeepSubtaskQueue<1>
{
	MT_DECLARE_TASK(DeepSubtaskQueue<1>, MT::StackRequirements::STANDARD, MT::TaskPriority::NORMAL, MT::Color::Blue);

	int result;
	void Do(MT::FiberContext&)
	{
		result = 1;
	}
};


//
TEST(DeepSubtaskQueue)
{
	MT::TaskScheduler scheduler;

	DeepSubtaskQueue<MT_SUBTASK_QUEUE_DEEP> task;
	scheduler.RunAsync(MT::TaskGroup::Default(), &task, 1);

	CHECK(scheduler.WaitAll(MT_DEFAULT_WAIT_TIME));
	CHECK_EQUAL(task.result, 144);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static MT::TaskGroup sourceGroup;
static MT::TaskGroup resultGroup;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct GroupSubtask
{
	MT_DECLARE_TASK(GroupSubtask, MT::StackRequirements::STANDARD, MT::TaskPriority::NORMAL, MT::Color::Blue);

	void Do(MT::FiberContext& context)
	{
		resultGroup = context.currentGroup;
	}
};

struct GroupTask
{
	MT_DECLARE_TASK(GroupTask, MT::StackRequirements::STANDARD, MT::TaskPriority::NORMAL, MT::Color::Blue);

	void Do(MT::FiberContext& context)
	{
		GroupSubtask task;
		context.RunSubtasksAndYield(sourceGroup, &task, 1);
	}
};

struct TaskWithManySubtasks
{
	MT_DECLARE_TASK(TaskWithManySubtasks, MT::StackRequirements::STANDARD, MT::TaskPriority::NORMAL, MT::Color::Blue);

	void Do(MT::FiberContext& context)
	{
		GroupTask task;
		for (int i = 0; i < 2; ++i)
		{
			context.RunSubtasksAndYield(MT::TaskGroup::Default(), &task, 1);
			MT::SpinSleepMilliSeconds(1);
		}
	}

};

//
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
#ifdef MT_INSTRUMENTED_BUILD
	MT::TaskScheduler scheduler(0, nullptr, GetProfiler());
#else
	MT::TaskScheduler scheduler;
#endif

	bool waitAllOK = true;

	sourceGroup = scheduler.CreateGroup();

	for (int i = 0; i < MT_ITERATIONS_COUNT; ++i)
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
	MT_DECLARE_TASK(TaskSubtaskCombo_Sum1, MT::StackRequirements::STANDARD, MT::TaskPriority::NORMAL, MT::Color::Blue);

	MT::Atomic32<int32>* data;

	void Do(MT::FiberContext&)
	{
		data->IncFetch();
	}
};

struct TaskSubtaskCombo_Sum4
{
	MT_DECLARE_TASK(TaskSubtaskCombo_Sum4, MT::StackRequirements::STANDARD, MT::TaskPriority::NORMAL, MT::Color::Blue);

	MT::Atomic32<int32>* data;

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
	MT_DECLARE_TASK(TaskSubtaskCombo_Sum16, MT::StackRequirements::STANDARD, MT::TaskPriority::NORMAL, MT::Color::Blue);

	MT::Atomic32<int32>* data;

	TaskSubtaskCombo_Sum4 tasks[2];

	void Do(MT::FiberContext& context)
	{
		tasks[0].data = data;
		tasks[1].data = data;

		context.RunAsync(MT::TaskGroup::Default(), &tasks[0], MT_ARRAY_SIZE(tasks));
		context.RunSubtasksAndYield(MT::TaskGroup::Default(), &tasks[0], MT_ARRAY_SIZE(tasks));
	}
};

MT::Atomic32<int32> sum;

//
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
