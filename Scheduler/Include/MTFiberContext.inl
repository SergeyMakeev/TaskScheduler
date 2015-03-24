
namespace MT
{

	template<class TTask>
	void FiberContext::RunSubtasksAndYield(TaskGroup::Type taskGroup, const TTask* taskArray, size_t taskCount)
	{
		ASSERT(threadContext, "ThreadContext is nullptr");
		ASSERT(taskCount < threadContext->descBuffer.size(), "Buffer overrun!");

		TaskScheduler& scheduler = *(threadContext->taskScheduler);

		fixed_array<internal::GroupedTask> buffer(&threadContext->descBuffer.front(), taskCount);

		size_t bucketCount = Min((size_t)scheduler.GetWorkerCount(), taskCount);
		fixed_array<internal::TaskBucket>	buckets(ALLOCATE_ON_STACK(sizeof(internal::TaskBucket) * bucketCount), bucketCount);

		internal::DistibuteDescriptions(taskGroup, taskArray, buffer, buckets);
		RunSubtasksAndYieldImpl(buckets);
	}

	template<class TTask>
	void FiberContext::RunAsync(TaskGroup::Type taskGroup, TTask* taskArray, size_t taskCount)
	{
		ASSERT(threadContext, "ThreadContext is nullptr");
		ASSERT(threadContext->taskScheduler->IsWorkerThread(), "Can't use RunAsync outside Task. Use TaskScheduler.RunAsync() instead.");

		TaskScheduler& scheduler = *(threadContext->taskScheduler);

		fixed_array<internal::GroupedTask> buffer(&threadContext->descBuffer.front(), taskCount);

		size_t bucketCount = Min((size_t)scheduler.GetWorkerCount(), taskCount);
		fixed_array<internal::TaskBucket>	buckets(ALLOCATE_ON_STACK(sizeof(internal::TaskBucket) * bucketCount), bucketCount);

		internal::DistibuteDescriptions(taskGroup, taskArray, buffer, buckets);
		scheduler.RunTasksImpl(buckets, nullptr, false);
	}




}
