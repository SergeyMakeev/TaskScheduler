#pragma once

#include <MTTools.h>
#include <MTPlatform.h>
#include <MTConcurrentQueueLIFO.h>
#include <MTTaskGroup.h>
#include <MTTaskDesc.h>

namespace MT
{
	class FiberContext;

	namespace internal
	{
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		struct GroupedTask
		{
			FiberContext* awaitingFiber;
			FiberContext* parentFiber;
			TaskGroup::Type group;
			TaskDesc desc;

			GroupedTask()
				: awaitingFiber(nullptr)
				, parentFiber(nullptr)
				, group(TaskGroup::GROUP_UNDEFINED)
			{}

			GroupedTask(TaskDesc& _desc, TaskGroup::Type _group)
				: awaitingFiber(nullptr)
				, parentFiber(nullptr)
				, group(_group)
				, desc(_desc)
			{}
		};
	}
}
