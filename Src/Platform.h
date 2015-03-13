#pragma once

#include "types.h"
#include "Debug.h"

#define MT_CALL_CONV __stdcall

#define ARRAY_SIZE( arr ) ( sizeof( arr ) / sizeof( (arr)[0] ) )

#if _MSC_VER

#define WIN32_LEAN_AND_MEAN
#include <windows.h>


namespace MT
{
	typedef HANDLE Thread;
	typedef void* Fiber;
	typedef void (MT_CALL_CONV *TFiberStartFunc)(void* userData);
	typedef uint32 (MT_CALL_CONV *TThreadStartFunc )(void* userData);

	inline void SwitchToFiber(MT::Fiber fiber)
	{
		::SwitchToFiber( (LPVOID)fiber );
	}

	inline MT::Fiber CreateFiber(size_t stackSize, TFiberStartFunc entryPoint, void* userData)
	{
		LPFIBER_START_ROUTINE pEntryPoint = (LPFIBER_START_ROUTINE)entryPoint;
		LPVOID fiber = ::CreateFiber( stackSize, pEntryPoint, userData );
		return fiber;
	}

	inline MT::Thread CreateSuspendedThread(size_t stackSize, TThreadStartFunc entryPoint, void* userData)
	{
		LPTHREAD_START_ROUTINE pEntryPoint = (LPTHREAD_START_ROUTINE)entryPoint;
		HANDLE thread = ::CreateThread( NULL, stackSize, pEntryPoint, userData, CREATE_SUSPENDED, NULL );
		return thread;
	}

	inline void ResumeThread(MT::Thread thread)
	{
		::ResumeThread(thread);
	}

	inline bool IsThreadAlive(MT::Thread thread)
	{
		DWORD exitCode;
		BOOL result = ::GetExitCodeThread( thread, &exitCode );
		return result && exitCode == STILL_ACTIVE;
	}

	inline bool CloseThread(MT::Thread thread, int waitTimeOutMS)
	{
		if (thread == NULL)
			return true;

		if (WAIT_OBJECT_0 != WaitForSingleObject(thread, waitTimeOutMS))
		{
			TerminateThread(thread, 1);
		}

		return CloseHandle(thread) == TRUE;
	}

	inline MT::Fiber ConvertCurrentThreadToFiber()
	{
		LPVOID fiber = ::ConvertThreadToFiberEx(NULL, FIBER_FLAG_FLOAT_SWITCH);
		return fiber;
	}

	inline void SetThreadProcessor(MT::Thread thread, uint32 processorNumber)
	{
		DWORD_PTR dwThreadAffinityMask = (1 << processorNumber);
		::SetThreadAffinityMask(thread, dwThreadAffinityMask);
	}

	inline uint32 GetCurrentThreadId()
	{
		return ::GetCurrentThreadId();
	}

	inline uint32 GetNumberOfHardwareThreads()
	{
		SYSTEM_INFO sysinfo;
		::GetSystemInfo( &sysinfo );
		return sysinfo.dwNumberOfProcessors;
	}

	namespace EventReset
	{
		enum Type
		{
			AUTOMATIC = 0,
			MANUAL = 1,
		};
	}

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

	class CriticalSection
	{
		::CRITICAL_SECTION criticalSection;

		CriticalSection( const CriticalSection & ) {}
		CriticalSection& operator=( const CriticalSection &) {}

		void Enter()
		{
			::EnterCriticalSection( &criticalSection );
		}
		void Leave()
		{
			::LeaveCriticalSection( &criticalSection );
		}
	public:
		CriticalSection()
		{
			::InitializeCriticalSection( &criticalSection );
		}

		~CriticalSection()
		{
			::DeleteCriticalSection( &criticalSection );
		}

		friend class MT::ScopedGuard;
	};



	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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


#elif __GNUC__

//GCC posix system

#include <pthread.h>


namespace MT
{
    typedef pthread_t Thread;

	inline MT::Thread CreateSuspendedThread(size_t stackSize, TThreadStartFunc entryPoint, void* userData)
	{
		LPTHREAD_START_ROUTINE pEntryPoint = (LPTHREAD_START_ROUTINE)entryPoint;

		int res = 0;

        pthread_attr_t threadAttr;
        res = pthread_attr_init(&threadAttr);
        ASSERT(res == 0, "Create thread error");

        res = pthread_attr_setstacksize(&threadAttr, stackSize);
        ASSERT(res == 0, "Create thread error");

		pthread_t thread;

        res = pthread_create(&thread, &threadAttr, pEntryPoint, userData);
        ASSERT(res == 0, "Create thread error");

		return thread;
	}


}

#else

#error "Platform not specified"

#endif



namespace MT
{

	class ScopedGuard
	{
		MT::CriticalSection & criticalSection;


		ScopedGuard( const ScopedGuard & ) : criticalSection(*((MT::CriticalSection*)nullptr)) {}
		ScopedGuard& operator=( const ScopedGuard &) {}

	public:

		ScopedGuard(MT::CriticalSection & cs) : criticalSection(cs)
		{
			criticalSection.Enter();
		}

		~ScopedGuard()
		{
			criticalSection.Leave();
		}
	};
}


