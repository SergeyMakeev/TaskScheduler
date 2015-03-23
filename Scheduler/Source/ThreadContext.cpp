#include <MTScheduler.h>

namespace MT
{
	static const size_t TASK_BUFFER_CAPACITY = 4096;

	ThreadContext::ThreadContext()
		: lastActiveFiberContext(nullptr)
		, taskScheduler(nullptr)
		, hasNewTasksEvent(EventReset::AUTOMATIC, true)
		, state(ThreadState::ALIVE)
		, descBuffer(TASK_BUFFER_CAPACITY)
	{
	}

	ThreadContext::~ThreadContext()
	{
	}

	void ThreadContext::RestoreAwaitingTasks(TaskGroup::Type taskGroup)
	{
		ASSERT(taskScheduler, "Invalid Task Scheduler");
		ASSERT(taskScheduler->IsWorkerThread(), "Can't use RunAsync outside Task. Use TaskScheduler.RunAsync() instead.");

		ConcurrentQueueLIFO<FiberContext*> & groupQueue = taskScheduler->waitTaskQueues[taskGroup];

		if (groupQueue.IsEmpty())
		{
			return;
		}

		//copy awaiting tasks list to stack
		stack_array<FiberContext*, MT_MAX_FIBERS_COUNT> groupQueueCopy(MT_MAX_FIBERS_COUNT, nullptr);
		size_t taskCount = groupQueue.PopAll(groupQueueCopy.begin(), groupQueueCopy.size());

		fixed_array<GroupedTask> buffer(&descBuffer.front(), taskCount);

		TaskScheduler & scheduler = *(taskScheduler);
		size_t bucketCount = Min((size_t)scheduler.GetWorkerCount(), taskCount);
		fixed_array<TaskBucket>	buckets(ALLOCATE_ON_STACK(TaskBucket, bucketCount), bucketCount);

		scheduler.DistibuteDescriptions(TaskGroup::GROUP_UNDEFINED, groupQueueCopy.begin(), buffer, buckets);
		scheduler.RunTasksImpl(buckets, nullptr, true);
	}



}
