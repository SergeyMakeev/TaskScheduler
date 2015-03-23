#pragma once

#include <MTTools.h>
#include <MTPlatform.h>
#include <MTConcurrentQueueLIFO.h>
#include <MTStackArray.h>
#include <MTFixedArray.h>


namespace MT
{
	namespace internal
	{
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		struct TaskBucket
		{
			GroupedTask* tasks;
			size_t count;
			TaskBucket(GroupedTask* _tasks, size_t _count)
				: tasks(_tasks)
				, count(_count)
			{
			}
		};
	}
}
