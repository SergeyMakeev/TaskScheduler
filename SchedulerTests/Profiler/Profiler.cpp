#include <MTScheduler.h>
#include "Profiler.h"


#ifdef MT_INSTRUMENTED_BUILD


void PushPerfMarker(const char* name, MT::Color::Type color)
{
	MT_UNUSED(name);
	MT_UNUSED(color);
}

void PopPerfMarker(const char* name)
{
	MT_UNUSED(name);
}



void PushPerfEvent(const char* name)
{
	PushPerfMarker(name, MT::Color::Red);
}

void PopPerfEvent(const char* name)
{
	PopPerfMarker(name);
}


class Microprofile : public MT::IProfilerEventListener
{
public:

	Microprofile()
	{
	}

	~Microprofile()
	{
	}

	virtual void OnFibersCreated(uint32 fibersCount) override
	{
		MT_UNUSED(fibersCount);
	}

	virtual void OnThreadsCreated(uint32 threadsCount) override
	{
		MT_UNUSED(threadsCount);
	}

	virtual void OnFiberAssignedToThread(uint32 fiberIndex, uint32 threadIndex) override
	{
		MT_UNUSED(fiberIndex);
		MT_UNUSED(threadIndex);
	}

	virtual void OnThreadAssignedToFiber(uint32 threadIndex, uint32 fiberIndex) override
	{
		MT_UNUSED(threadIndex);
		MT_UNUSED(fiberIndex);
	}

	virtual void OnThreadCreated(uint32 workerIndex) override 
	{
		MT_UNUSED(workerIndex);
	}

	virtual void OnThreadStarted(uint32 workerIndex) override 
	{
		MT_UNUSED(workerIndex);
	}

	virtual void OnThreadStoped(uint32 workerIndex) override 
	{
		MT_UNUSED(workerIndex);
	}

	virtual void OnThreadIdleStarted(uint32 workerIndex) override 
	{
		MT_UNUSED(workerIndex);
		PushPerfMarker("ThreadIdle", MT::Color::Red);
	}

	virtual void OnThreadIdleFinished(uint32 workerIndex) override 
	{
		MT_UNUSED(workerIndex);
		PopPerfMarker("ThreadIdle");
	}

	virtual void OnThreadWaitStarted() override 
	{
		PushPerfMarker("ThreadWait", MT::Color::Red);
	}

	virtual void OnThreadWaitFinished() override 
	{
		PopPerfMarker("ThreadWait");
	}

	virtual void OnTaskExecuteStateChanged(MT::Color::Type debugColor, const mt_char* debugID, MT::TaskExecuteState::Type type) override 
	{
		switch(type)
		{
		case MT::TaskExecuteState::START:
		case MT::TaskExecuteState::RESUME:
			PushPerfMarker(debugID, debugColor);
			break;
		case MT::TaskExecuteState::STOP:
		case MT::TaskExecuteState::SUSPEND:
			PopPerfMarker(debugID);
			break;
		}
	}
};


#endif


MT::IProfilerEventListener* GetProfiler()
{
#ifdef MT_INSTRUMENTED_BUILD
	static Microprofile profile;
	return &profile;
#else
	return nullptr;
#endif

	
}
