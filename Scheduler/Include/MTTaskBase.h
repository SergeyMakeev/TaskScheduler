#pragma once

#include <MTTools.h>
#include <MTPlatform.h>
#include <MTConcurrentQueueLIFO.h>
#include <MTStackArray.h>
#include <MTFixedArray.h>


namespace MT
{
		template<typename T>
		struct TaskBase
		{
			static void TaskEntryPoint(MT::FiberContext& fiberContext, void* userData)
			{
				T* task = static_cast<T*>(userData);
				task->Do(fiberContext);
			}
		};
}
