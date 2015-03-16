#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define MT_CALL_CONV __stdcall

#include "ThreadWindows.h"

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


	//
	//
	//
	class Event
	{
		::HANDLE eventHandle;

	public:

		Event()
		{
			static_assert(sizeof(Event) == sizeof(::HANDLE), "sizeof(Event) is invalid");
			eventHandle = NULL;
		}

		Event(EventReset::Type resetType, bool initialState)
		{
			eventHandle = NULL;
			Create(resetType, initialState);
		}

		~Event()
		{
			CloseHandle(eventHandle);
			eventHandle = NULL;
		}

		void Create(EventReset::Type resetType, bool initialState)
		{
			if (eventHandle != NULL)
			{
				CloseHandle(eventHandle);
			}

			BOOL bManualReset = (resetType == EventReset::AUTOMATIC) ? FALSE : TRUE;
			BOOL bInitialState = initialState ? TRUE : FALSE;
			eventHandle = ::CreateEvent(NULL, bManualReset, bInitialState, NULL);
		}

		void Signal()
		{
			SetEvent(eventHandle);
		}

		void Reset()
		{
			ResetEvent(eventHandle);
		}

		bool Wait(uint32 milliseconds)
		{
			DWORD res = WaitForSingleObject(eventHandle, milliseconds);
			return (res == WAIT_OBJECT_0);
		}

		bool Check()
		{
			return Wait(0);
		}

		static inline bool WaitAll(Event * eventsArray, uint32 size, uint32 milliseconds)
		{
			DWORD res = WaitForMultipleObjects(size, (::HANDLE *)&eventsArray[0], TRUE, milliseconds);
			return (res == WAIT_OBJECT_0);
		}
	};

	class ScopedGuard;

	//
	// 
	//
	class Mutex
	{
		::CRITICAL_SECTION criticalSection;

		Mutex( const Mutex & ) {}
		Mutex& operator=( const Mutex &) {}

		void Lock()
		{
			::EnterCriticalSection( &criticalSection );
		}
		void Unlock()
		{
			::LeaveCriticalSection( &criticalSection );
		}
	public:
		Mutex()
		{
			::InitializeCriticalSection( &criticalSection );
		}

		~Mutex()
		{
			::DeleteCriticalSection( &criticalSection );
		}

		friend class MT::ScopedGuard;
	};


	//
	// Atomic int
	//
	class AtomicInt
	{
		volatile long value;
	public:

		AtomicInt() : value(0)
		{
		}

		AtomicInt(int v)
			: value(v)
		{
		}

		void Add(int sum)
		{
			InterlockedExchangeAdd(&value, sum);
		}

		int Inc()
		{
			return InterlockedIncrement(&value);
		}

		int Dec()
		{
			return InterlockedDecrement(&value);
		}

		int Get()
		{
			return value;
		}

		int Set(int val)
		{
			return InterlockedExchange(&value, val); 
		}
	};

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


