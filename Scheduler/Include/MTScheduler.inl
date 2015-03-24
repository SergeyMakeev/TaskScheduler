
namespace MT
{

	namespace internal
	{
		template<class T>
		inline internal::GroupedTask GetGroupedTask(TaskGroup::Type group, T * src)
		{
			internal::TaskDesc desc(T::TaskEntryPoint, (void*)(src));
			return internal::GroupedTask(desc, group);
		}

		//template specialization for FiberContext*
		template<>
		inline internal::GroupedTask GetGroupedTask(TaskGroup::Type group, FiberContext ** src)
		{
			ASSERT(group == TaskGroup::GROUP_UNDEFINED, "Group must be GROUP_UNDEFINED");
			FiberContext * fiberContext = *src;
			internal::GroupedTask groupedTask(fiberContext->currentTask, fiberContext->currentGroup);
			groupedTask.awaitingFiber = fiberContext;
			return groupedTask;
		}


		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Distributes task to threads:
		// | Task1 | Task2 | Task3 | Task4 | Task5 | Task6 |
		// ThreadCount = 4
		// Thread0: Task1, Task5
		// Thread1: Task2, Task6
		// Thread2: Task3
		// Thread3: Task4
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		template<class TTask>
		inline bool DistibuteDescriptions(TaskGroup::Type group, TTask* taskArray, fixed_array<internal::GroupedTask>& descriptions, fixed_array<internal::TaskBucket>& buckets)
		{
			size_t index = 0;

			for (size_t bucketIndex = 0; (bucketIndex < buckets.size()) && (index < descriptions.size()); ++bucketIndex)
			{
				size_t bucketStartIndex = index;

				for (size_t i = bucketIndex; i < descriptions.size(); i += buckets.size())
				{
					descriptions[index++] = GetGroupedTask(group, &taskArray[i]);
				}

				buckets[bucketIndex] = internal::TaskBucket(&descriptions[bucketStartIndex], index - bucketStartIndex);
			}

			ASSERT(index == descriptions.size(), "Sanity check");
			return index > 0;
		}

	}





	template<class TTask>
	void TaskScheduler::RunAsync(TaskGroup::Type group, TTask* taskArray, uint32 taskCount)
	{
		ASSERT(!IsWorkerThread(), "Can't use RunAsync inside Task. Use FiberContext.RunAsync() instead.");

		fixed_array<internal::GroupedTask> buffer(ALLOCATE_ON_STACK(sizeof(internal::GroupedTask) * taskCount), taskCount);

		size_t bucketCount = Min(threadsCount, taskCount);
		fixed_array<internal::TaskBucket>	buckets(ALLOCATE_ON_STACK(sizeof(internal::TaskBucket) * bucketCount), bucketCount);

		internal::DistibuteDescriptions(group, taskArray, buffer, buckets);
		RunTasksImpl(buckets, nullptr, false);
	}

}
