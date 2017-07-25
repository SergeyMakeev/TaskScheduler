#pragma once

// Additional Include Directories and Additional Library Directories must be configured to Brofiler
//#define MT_ENABLE_BROFILER_SUPPORT (1)

#if defined(MT_INSTRUMENTED_BUILD) && defined(MT_ENABLE_BROFILER_SUPPORT)
#include <Brofiler.h>

#define BROFILER_NEXT_FRAME() Brofiler::NextFrame();   \
                              BROFILER_EVENT("Frame") \


#else
#define PROFILE
#define BROFILER_INLINE_EVENT(NAME, CODE) { CODE; }
#define BROFILER_CATEGORY(NAME, COLOR)
#define BROFILER_FRAME(NAME)
#define BROFILER_THREAD(FRAME_NAME)
#define BROFILER_NEXT_FRAME()
#endif


namespace MT
{
	class IProfilerEventListener;
}


MT::IProfilerEventListener* GetProfiler();




