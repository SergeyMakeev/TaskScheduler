#pragma once

#include <MTTools.h>
#include <MTPlatform.h>
#include <MTConcurrentQueueLIFO.h>
#include <MTStackArray.h>
#include <MTFixedArray.h>


namespace MT
{
	class FiberContext;
	typedef void (*TTaskEntryPoint)(FiberContext & context, void* userData);

	namespace internal
	{
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Task description
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		struct TaskDesc
		{
			//Task entry point
			TTaskEntryPoint taskFunc;

			//Task user data (task context)
			void* userData;

			TaskDesc()
				: taskFunc(nullptr)
				, userData(nullptr)
			{
			}

			TaskDesc(TTaskEntryPoint _taskFunc, void* _userData)
				: taskFunc(_taskFunc)
				, userData(_userData)
			{
			}

			bool IsValid()
			{
				return (taskFunc != nullptr);
			}
		};
	}

}
