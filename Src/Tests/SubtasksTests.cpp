#include "Tests.h"
#include "../../TestFramework/UnitTest++/UnitTest++.h"
#include "../Scheduler.h"


SUITE(SubtasksTests)
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace DeepSubtaskQueue
{
	template<size_t N>
	void MT_CALL_CONV Fibonacci(MT::FiberContext& context, void* userData)
	{
		int& result = *(int*)userData;

		MT::TaskDesc tasks[2];

		int a = -100;
		tasks[0] = MT::TaskDesc(Fibonacci<N - 1>, &a);

		int b = -100;
		tasks[1] = MT::TaskDesc(Fibonacci<N - 2>, &b);

		context.RunSubtasksAndYield(&tasks[0], ARRAY_SIZE(tasks));

		result = a + b;
	}

	template<>
	void MT_CALL_CONV Fibonacci<0>(MT::FiberContext&, void* userData)
	{
		*(int*)userData = 0;
	}

	template<>
	void MT_CALL_CONV Fibonacci<1>(MT::FiberContext&, void* userData)
	{
		*(int*)userData = 1;
	}
}

// Checks one simple task
TEST(DeepSubtaskQueue)
{
	MT::TaskScheduler scheduler;

	int result = 0;
	MT::TaskDesc task(DeepSubtaskQueue::Fibonacci<12>, &result);

	scheduler.RunTasks(MT::TaskGroup::GROUP_0, &task, 1);

	CHECK(scheduler.WaitAll(200));

	CHECK_EQUAL(result, 144);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace SubtaskGroup
{
	static MT::TaskGroup::Type sourceGroup = MT::TaskGroup::GROUP_1;
	static MT::TaskGroup::Type resultGroup = MT::TaskGroup::GROUP_UNDEFINED;

	void MT_CALL_CONV Subtask(MT::FiberContext& context, void*)
	{
		resultGroup = context.currentTask->GetGroup();
	}

	void MT_CALL_CONV Task(MT::FiberContext& context, void*)
	{
		MT::TaskDesc task(Subtask, 0);
		context.RunSubtasksAndYield(&task, 1);
	}

	void MT_CALL_CONV TaskWithManySubtasks(MT::FiberContext& context, void*)
	{
		MT::TaskDesc task(Subtask, 0);
		for (int i = 0; i < 2; ++i)
		{
			ASSERT(context.currentTask->GetGroup() < MT::TaskGroup::COUNT, "Invalid group");

			context.RunSubtasksAndYield(&task, 1);

			ASSERT(context.currentTask->GetGroup() < MT::TaskGroup::COUNT, "Invalid group");

			Sleep(1);
		}
	}


	// Checks one simple task
	TEST(SubtaskGroup)
	{
		MT::TaskScheduler scheduler;

		MT::TaskDesc task(SubtaskGroup::Task, 0);
		scheduler.RunTasks(SubtaskGroup::sourceGroup, &task, 1);

		CHECK(scheduler.WaitAll(200));

		CHECK_EQUAL(SubtaskGroup::sourceGroup, SubtaskGroup::resultGroup);
	}

	// Checks task with multiple subtasks
	TEST(OneTaskManySubtasks)
	{
		MT::TaskScheduler scheduler;

		MT::TaskDesc task(SubtaskGroup::TaskWithManySubtasks, 0);
		scheduler.RunTasks(SubtaskGroup::sourceGroup, &task, 1);

		CHECK(scheduler.WaitAll(100));
	}

	// Checks many simple task with subtasks
	TEST(ManyTasksOneSubtask)
	{
		MT::TaskScheduler scheduler;

		bool waitAllOK = true;

		for (int i = 0; i < 100000 && waitAllOK; ++i)
		{
			MT::TaskDesc task(SubtaskGroup::Task, 0);
			scheduler.RunTasks(SubtaskGroup::sourceGroup, &task, 1);
			waitAllOK = waitAllOK && scheduler.WaitAll(200);
		}

		CHECK(waitAllOK);
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace TaskSubtaskCombo
{
	static MT::AtomicInt sum;

	void MT_CALL_CONV Function_Sum1(MT::FiberContext&, void* userData)
	{
		MT::AtomicInt& data = *(MT::AtomicInt*)userData;
		data.Inc();
	}

	void MT_CALL_CONV Function_Sum4(MT::FiberContext& context, void* userData)
	{
		MT::TaskDesc tasks[] = { MT::TaskDesc(Function_Sum1, userData), MT::TaskDesc(Function_Sum1, userData) };
		context.threadContext->taskScheduler->RunTasks(MT::TaskGroup::GROUP_2, &tasks[0], ARRAY_SIZE(tasks));
		context.RunSubtasksAndYield(&tasks[0], ARRAY_SIZE(tasks));
	}

	void MT_CALL_CONV Function_Sum16(MT::FiberContext& context, void* userData)
	{
		MT::TaskDesc tasks[] = { MT::TaskDesc(Function_Sum4, userData), MT::TaskDesc(Function_Sum4, userData) };
		context.threadContext->taskScheduler->RunTasks(MT::TaskGroup::GROUP_1, &tasks[0], ARRAY_SIZE(tasks));
		context.RunSubtasksAndYield(&tasks[0], ARRAY_SIZE(tasks));
	}

	void MT_CALL_CONV RootTask_Sum256(MT::FiberContext& context, void* userData)
	{
		for (int i = 0; i < 16; ++i)
		{
			MT::TaskDesc task(Function_Sum16, userData);
			context.RunSubtasksAndYield(&task, 1);
		}
	}

	// Checks one simple task
	TEST(TaskSubtaskCombo)
	{
		MT::TaskScheduler scheduler;

		MT::AtomicInt sum;

		MT::TaskDesc task(TaskSubtaskCombo::RootTask_Sum256, &sum);
		scheduler.RunTasks(MT::TaskGroup::GROUP_0, &task, 1);

		CHECK(scheduler.WaitAll(200));

		CHECK_EQUAL(sum.Get(), 256);
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}