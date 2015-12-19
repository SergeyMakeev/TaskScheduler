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

#pragma once

#include <MTTools.h>
#include <MTPlatform.h>
#include <MTConcurrentQueueLIFO.h>
#include <MTStackArray.h>
#include <MTArrayView.h>
#include <MTThreadContext.h>
#include <MTFiberContext.h>
#include <MTAllocator.h>
#include <MTTaskPool.h>


namespace MT
{

	template<typename CLASS_TYPE, typename MACRO_TYPE>
	struct CheckType
	{
		static_assert(std::is_same<CLASS_TYPE, MACRO_TYPE>::value, "Invalid type in MT_DECLARE_TASK macro. See CheckType template instantiation params to details.");
	};

	struct TypeChecker
	{
		template <typename T>
		static T QueryThisType(T thisPtr)
		{
			return (T)nullptr;
		}
	};


	template <typename T>
	inline void CallDtor(T * p)
	{
#if _MSC_VER
		p;
#endif
		p->~T();
	}

}

#define MT_COLOR_DEFAULT (0)
#define MT_COLOR_BLUE (1)
#define MT_COLOR_RED (2)
#define MT_COLOR_YELLOW (3)

#if _MSC_VER

// Visual Studio compile time check
#define COMPILE_TIME_TYPE_CHECK(TYPE) \
	void CompileTimeCheckMethod() \
	{ \
		MT::CheckType< typename std::remove_pointer< decltype(MT::TypeChecker::QueryThisType(this)) >::type, typename TYPE > compileTypeTypesCheck; \
		compileTypeTypesCheck; \
	}

#else

// GCC, Clang and other compilers compile time check
#define COMPILE_TIME_TYPE_CHECK(TYPE) \
	void CompileTimeCheckMethod() \
	{ \
		/* query this pointer type */ \
		typedef decltype(MT::TypeChecker::QueryThisType(this)) THIS_PTR_TYPE; \
		/* query class type from this pointer type */ \
		typedef typename std::remove_pointer<THIS_PTR_TYPE>::type CPP_TYPE; \
		/* define macro type */ \
		typedef TYPE MACRO_TYPE; \
		/* compile time checking that is same types */ \
		MT::CheckType< CPP_TYPE, MACRO_TYPE > compileTypeTypesCheck; \
		/* remove unused variable warning */ \
		compileTypeTypesCheck; \
	}

#endif




#define MT_DECLARE_TASK_IMPL(TYPE) \
	\
	COMPILE_TIME_TYPE_CHECK(TYPE) \
	\
	static void TaskEntryPoint(MT::FiberContext& fiberContext, void* userData) \
	{ \
		TYPE * task = static_cast< TYPE *>(userData); \
		task->Do(fiberContext); \
	} \
	\
	static void PoolTaskDestroy(void* userData) \
	{ \
		TYPE * task = static_cast< TYPE *>(userData); \
		MT::CallDtor( task ); \
		/* Find task pool header */ \
		MT::PoolElementHeader * poolHeader = (MT::PoolElementHeader *)((char*)userData - sizeof(MT::PoolElementHeader)); \
		/* Fixup pool header, mark task as unused */ \
		poolHeader->id.Store(MT::TaskID::UNUSED); \
	} \



#ifdef MT_INSTRUMENTED_BUILD
#include <MTMicroWebSrv.h>
#include <MTProfilerEventListener.h>

#define MT_DECLARE_TASK(TYPE, colorID) \
	static const mt_char* GetDebugID() \
	{ \
		return MT_TEXT( #TYPE ); \
	} \
	\
	static int GetDebugColorIndex() \
	{ \
		return colorID; \
	} \
	\
	MT_DECLARE_TASK_IMPL(TYPE);


#else

#define MT_DECLARE_TASK(TYPE, colorID) \
	MT_DECLARE_TASK_IMPL(TYPE);

#endif






namespace MT
{
	const uint32 MT_MAX_THREAD_COUNT = 64;
	const uint32 MT_MAX_FIBERS_COUNT = 256;
	const uint32 MT_SCHEDULER_STACK_SIZE = 1048576;
	const uint32 MT_FIBER_STACK_SIZE = 65536;

	namespace internal
	{
		struct ThreadContext;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Task scheduler
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class TaskScheduler
	{
		friend class FiberContext;
		friend struct internal::ThreadContext;



		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Task group description
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Application can assign task group to task and later wait until group was finished.
		class TaskGroupDescription
		{
			AtomicInt32 inProgressTaskCount;
			Event allDoneEvent;

			//Tasks awaiting group through FiberContext::WaitGroupAndYield call
			ConcurrentQueueLIFO<FiberContext*> waitTasksQueue;

		public:

			bool debugIsFree;


		private:

			TaskGroupDescription(TaskGroupDescription& ) {}
			void operator=(const TaskGroupDescription&) {}

		public:

			TaskGroupDescription()
			{
				inProgressTaskCount.Store(0);
				allDoneEvent.Create( EventReset::MANUAL, true );
				debugIsFree = true;
			}

			int GetTaskCount() const
			{
				return inProgressTaskCount.Load();
			}

			ConcurrentQueueLIFO<FiberContext*> & GetWaitQueue()
			{
				return waitTasksQueue;
			}

			int Dec()
			{
				return inProgressTaskCount.DecFetch();
			}

			int Inc()
			{
				return inProgressTaskCount.IncFetch();
			}

			int Add(int sum)
			{
				return inProgressTaskCount.AddFetch(sum);
			}

			void Signal()
			{
				allDoneEvent.Signal();
			}

			void Reset()
			{
				allDoneEvent.Reset();
			}

			bool Wait(uint32 milliseconds)
			{
				return allDoneEvent.Wait(milliseconds);
			}
		};


		// Thread index for new task
		AtomicInt32 roundRobinThreadIndex;

		// Started threads count
		AtomicInt32 startedThreadsCount;

		// Threads created by task manager
		volatile uint32 threadsCount;
		internal::ThreadContext threadContext[MT_MAX_THREAD_COUNT];

		// All groups task statistic
		TaskGroupDescription allGroups;

		// Groups pool
		ConcurrentQueueLIFO<TaskGroup> availableGroups;

		//
		TaskGroupDescription groupStats[TaskGroup::MT_MAX_GROUPS_COUNT];

		// Fibers pool
		ConcurrentQueueLIFO<FiberContext*> availableFibers;

		// Fibers context
		FiberContext fiberContext[MT_MAX_FIBERS_COUNT];

#ifdef MT_INSTRUMENTED_BUILD
		IProfilerEventListener * profilerEventListener;
		int64 startTime;
		profile::MicroWebServer profilerWebServer;
		int32 webServerPort;
#endif

		FiberContext* RequestFiberContext(internal::GroupedTask& task);
		void ReleaseFiberContext(FiberContext* fiberExecutionContext);
		void RunTasksImpl(ArrayView<internal::TaskBucket>& buckets, FiberContext * parentFiber, bool restoredFromAwaitState);
		TaskGroupDescription & GetGroupDesc(TaskGroup group);

		static void ThreadMain( void* userData );
		static void FiberMain( void* userData );
		static bool TryStealTask(internal::ThreadContext& threadContext, internal::GroupedTask & task, uint32 workersCount);

		static FiberContext* ExecuteTask (internal::ThreadContext& threadContext, FiberContext* fiberContext);

	public:

		/// \brief Initializes a new instance of the TaskScheduler class.
		/// \param workerThreadsCount Worker threads count. Automatically determines the required number of threads if workerThreadsCount set to 0
#ifdef MT_INSTRUMENTED_BUILD
		TaskScheduler(uint32 workerThreadsCount = 0, IProfilerEventListener* listener = nullptr);
#else
		TaskScheduler(uint32 workerThreadsCount = 0);
#endif


		~TaskScheduler();

		template<class TTask>
		void RunAsync(TaskGroup group, TTask* taskArray, uint32 taskCount);

		void RunAsync(TaskGroup group, TaskHandle* taskHandleArray, uint32 taskHandleCount);


		bool WaitGroup(TaskGroup group, uint32 milliseconds);
		bool WaitAll(uint32 milliseconds);

		TaskGroup CreateGroup();
		void ReleaseGroup(TaskGroup group);

		bool IsEmpty();

		uint32 GetWorkerCount() const;

		bool IsWorkerThread() const;

#ifdef MT_INSTRUMENTED_BUILD

		size_t GetProfilerEvents(uint32 workerIndex, MT::ProfileEventDesc * dstBuffer, size_t dstBufferSize);
		void UpdateProfiler();
		int32 GetWebServerPort() const;

		inline int64 GetStartTime() const
		{
			return startTime;
		}

		inline uint64 GetTimeStamp() const
		{
			return MT::GetTimeMicroSeconds() - startTime;
		}

		inline IProfilerEventListener* GetProfilerEventListener()
		{
			return profilerEventListener;
		}		

#endif
	};
}

#include "MTScheduler.inl"
#include "MTFiberContext.inl"
