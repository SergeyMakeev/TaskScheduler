#pragma once


namespace MT
{
	class IProfilerEventListener;
}


MT::IProfilerEventListener* GetProfiler();

#ifdef MT_INSTRUMENTED_BUILD

void PushPerfEvent(const char* name);
void PopPerfEvent(const char* name);

#endif
