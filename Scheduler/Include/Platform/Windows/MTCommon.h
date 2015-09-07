#pragma once

#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif


#ifdef _WIN32_WINNT
	#undef _WIN32_WINNT
#endif

#define _WIN32_WINNT 0x0400

#include <windows.h>

#include "MTUtils.h"
#include "MTThread.h"
#include "MTMutex.h"
#include "MTAtomic.h"
#include "MTEvent.h"
#include "MTFiber.h"
#include "MTMemory.h"
