#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define MT_CALL_CONV __stdcall

#include "Thread.h"
#include "Mutex.h"
#include "Atomic.h"
#include "Event.h"

namespace MT
{
	typedef void* Fiber;
	typedef void (MT_CALL_CONV *TFiberStartFunc)(void* userData);

	//
	//
	//
	inline void SwitchToFiber(MT::Fiber fiber)
	{
		ASSERT(fiber != nullptr, "Invalid fiber to switch");
		::SwitchToFiber( (LPVOID)fiber );
	}

	//
	//
	//
	inline MT::Fiber CreateFiber(size_t stackSize, TFiberStartFunc entryPoint, void* userData)
	{
		LPFIBER_START_ROUTINE pEntryPoint = (LPFIBER_START_ROUTINE)entryPoint;
		LPVOID fiber = ::CreateFiber( stackSize, pEntryPoint, userData );
		return fiber;
	}

	//
	//
	//
	inline MT::Fiber ConvertCurrentThreadToFiber()
	{
		LPVOID fiber = ::ConvertThreadToFiberEx(NULL, FIBER_FLAG_FLOAT_SWITCH);
		return fiber;
	}



}


namespace Time
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline int64 GetTime()
{
	LARGE_INTEGER largeInteger;
	QueryPerformanceCounter( &largeInteger );
	return largeInteger.QuadPart;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline int64 GetFrequency()
{
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	return frequency.QuadPart;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline int64 GetTimeMilliSeconds()
{
	LARGE_INTEGER largeInteger;
	QueryPerformanceCounter( &largeInteger );
	return ( largeInteger.QuadPart * int64(1000) ) / GetFrequency();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline int64 GetTimeMicroSeconds()
{
	LARGE_INTEGER largeInteger;
	QueryPerformanceCounter( &largeInteger );
	return ( largeInteger.QuadPart * int64(1000000) ) / GetFrequency();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline void SpinSleep(uint32 microSeconds)
{
	int64 time = GetTimeMicroSeconds() + microSeconds;
	while(GetTimeMicroSeconds() < time)
	{
		Sleep(0);
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


