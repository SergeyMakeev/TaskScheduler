#pragma once

#include <MTTools.h>
#include <MTPlatform.h>
#include <MTConcurrentQueueLIFO.h>
#include <MTGroupedTask.h>

namespace MT
{
	class FiberContext;
	class TaskScheduler;

	namespace internal
	{

		namespace ThreadState
		{
			const uint32 ALIVE = 0;
			const uint32 EXIT = 1;
		};

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Thread (Scheduler fiber) context
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		struct ThreadContext
		{
			FiberContext* lastActiveFiberContext;

			// pointer to task manager
			TaskScheduler* taskScheduler;

			// thread
			Thread thread;

			// scheduler fiber
			Fiber schedulerFiber;

			// task queue awaiting execution
			ConcurrentQueueLIFO<internal::GroupedTask> queue;

			// new task was arrived to queue event
			Event hasNewTasksEvent;

			// whether thread is alive
			AtomicInt state;

			// Temporary buffer
			std::vector<internal::GroupedTask> descBuffer;

			// prevent false sharing between threads
			uint8 cacheline[64];

			ThreadContext();
			~ThreadContext();

			void RestoreAwaitingTasks(TaskGroup::Type taskGroup);
		};

	}

}
