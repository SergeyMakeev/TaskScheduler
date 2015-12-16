#pragma once

#ifndef __MT_COMMON__
#define __MT_COMMON__


#include "MTAtomic.h"


#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif


#ifdef _WIN32_WINNT
	#undef _WIN32_WINNT
#endif

#define _WIN32_WINNT 0x0403

#include <windows.h>

#include "MTUtils.h"
#include "MTThread.h"
#include "MTMutex.h"
#include "MTEvent.h"
#include "MTFiber.h"
#include "MTMemory.h"


#endif