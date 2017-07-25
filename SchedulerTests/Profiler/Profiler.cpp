#include <MTScheduler.h>
#include "Profiler.h"


#if defined(MT_INSTRUMENTED_BUILD) && defined(MT_ENABLE_BROFILER_SUPPORT)

#pragma comment( lib, "BrofilerCore.lib" )
#pragma comment( lib, "Advapi32.lib" )


const uint32 MAX_INSTRUMENTED_WORKERS = 8;
const char *g_WorkerNames[MAX_INSTRUMENTED_WORKERS] = {"worker0","worker1","worker2","worker3","worker4","worker5","worker6","worker7"};

class ProfilerEventListener : public MT::IProfilerEventListener
{
	Brofiler::EventStorage* fiberEventStorages[MT::MT_MAX_STANDART_FIBERS_COUNT + MT::MT_MAX_EXTENDED_FIBERS_COUNT];
	uint32 totalFibersCount;

	static mt_thread_local Brofiler::EventStorage* originalThreadStorage;
	static mt_thread_local Brofiler::EventStorage* activeThreadStorage;

public:

	ProfilerEventListener()
		: totalFibersCount(0)
	{
	}

	virtual void OnFibersCreated(uint32 fibersCount) override
	{
		totalFibersCount = fibersCount;
		MT_ASSERT(fibersCount <= MT_ARRAY_SIZE(fiberEventStorages), "Too many fibers!");
		for(uint32 fiberIndex = 0; fiberIndex < fibersCount; fiberIndex++)
		{
			Brofiler::RegisterFiber(fiberIndex, &fiberEventStorages[fiberIndex]);
		}
	}

	virtual void OnThreadsCreated(uint32 threadsCount) override
	{
		MT_UNUSED(threadsCount);
	}

	virtual void OnTemporaryWorkerThreadLeave() override
	{
		Brofiler::EventStorage** currentThreadStorageSlot = Brofiler::GetEventStorageSlotForCurrentThread();
		MT_ASSERT(currentThreadStorageSlot, "Sanity check failed");
		Brofiler::EventStorage* storage = *currentThreadStorageSlot;

		// if profile session is not active
		if (storage == nullptr)
		{
			return;
		}

		MT_ASSERT(IsFiberStorage(storage) == false, "Sanity check failed");
	}

	virtual void OnTemporaryWorkerThreadJoin() override
	{
		Brofiler::EventStorage** currentThreadStorageSlot = Brofiler::GetEventStorageSlotForCurrentThread();
		MT_ASSERT(currentThreadStorageSlot, "Sanity check failed");
		Brofiler::EventStorage* storage = *currentThreadStorageSlot;

		// if profile session is not active
		if (storage == nullptr)
		{
			return;
		}

		MT_ASSERT(IsFiberStorage(storage) == false, "Sanity check failed");
	}


	virtual void OnThreadCreated(uint32 workerIndex) override 
	{
		BROFILER_START_THREAD("Scheduler(Worker)");
		MT_UNUSED(workerIndex);
	}

	virtual void OnThreadStarted(uint32 workerIndex) override
	{
		MT_UNUSED(workerIndex);
	}

	virtual void OnThreadStoped(uint32 workerIndex) override
	{
		MT_UNUSED(workerIndex);
		BROFILER_STOP_THREAD();
	}

	virtual void OnThreadIdleStarted(uint32 workerIndex) override
	{
		MT_UNUSED(workerIndex);
	}

	virtual void OnThreadIdleFinished(uint32 workerIndex) override
	{
		MT_UNUSED(workerIndex);
	}

	virtual void OnThreadWaitStarted() override
	{
	}

	virtual void OnThreadWaitFinished() override
	{
	}

	virtual void OnTaskExecuteStateChanged(MT::Color::Type debugColor, const mt_char* debugID, MT::TaskExecuteState::Type type, int32 fiberIndex) override 
	{
		MT_UNUSED(debugColor);
		MT_UNUSED(debugID);
		//MT_UNUSED(type);

		MT_ASSERT(fiberIndex < (int32)totalFibersCount, "Sanity check failed");

		Brofiler::EventStorage** currentThreadStorageSlot = Brofiler::GetEventStorageSlotForCurrentThread();
		MT_ASSERT(currentThreadStorageSlot, "Sanity check failed");

		// if profile session is not active
		if (*currentThreadStorageSlot == nullptr)
		{
			return;
		}

		// if actual fiber is scheduler internal fiber (don't have event storage for internal scheduler fibers)
		if (fiberIndex < 0)
		{
			return;
		}

		switch(type)
		{
		case MT::TaskExecuteState::START:
		case MT::TaskExecuteState::RESUME:
			{
				MT_ASSERT(originalThreadStorage == nullptr, "Sanity check failed");

				originalThreadStorage = *currentThreadStorageSlot;

				MT_ASSERT(IsFiberStorage(originalThreadStorage) == false, "Sanity check failed");

				Brofiler::EventStorage* currentFiberStorage = nullptr;
				if (fiberIndex >= (int32)0)
				{
					currentFiberStorage = fiberEventStorages[fiberIndex];
				} 

				*currentThreadStorageSlot = currentFiberStorage;
				activeThreadStorage = currentFiberStorage;
				Brofiler::FiberSyncData::AttachToThread(currentFiberStorage, MT::ThreadId::Self().AsUInt64());
			}
			break;

		case MT::TaskExecuteState::STOP:
		case MT::TaskExecuteState::SUSPEND:
			{
				Brofiler::EventStorage* currentFiberStorage = *currentThreadStorageSlot;

				//////////////////////////////////////////////////////////////////////////
				Brofiler::EventStorage* checkFiberStorage = nullptr;
				if (fiberIndex >= (int32)0)
				{
					checkFiberStorage = fiberEventStorages[fiberIndex];
				}
				MT_ASSERT(checkFiberStorage == currentFiberStorage, "Sanity check failed");

				MT_ASSERT(activeThreadStorage == currentFiberStorage, "Sanity check failed");

				//////////////////////////////////////////////////////////////////////////

				MT_ASSERT(IsFiberStorage(currentFiberStorage) == true, "Sanity check failed");

				Brofiler::FiberSyncData::DetachFromThread(currentFiberStorage);

				*currentThreadStorageSlot = originalThreadStorage;
				originalThreadStorage = nullptr;
			}
			break;
		}
	}
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
mt_thread_local Brofiler::EventStorage* ProfilerEventListener::originalThreadStorage = nullptr;
mt_thread_local Brofiler::EventStorage* ProfilerEventListener::activeThreadStorage = 0;

#endif


MT::IProfilerEventListener* GetProfiler()
{
#if defined(MT_INSTRUMENTED_BUILD) && defined(MT_ENABLE_BROFILER_SUPPORT)
	static ProfilerEventListener profilerListener;
	return &profilerListener;
#else
	return nullptr;
#endif
}

