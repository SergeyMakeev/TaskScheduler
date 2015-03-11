#pragma once

#include <windows.h>
#include "types.h"
#include "Assert.h"

#define MT_CALL_CONV __stdcall
#define INLINE __forceinline

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

	inline uint32 GetNumberOfProcessors()
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
	};

	inline bool WaitForMultipleEvents(Event * eventsArray, uint32 size, uint32 milliseconds)
	{
		//can cast Event to ::HANDLE because sizeof(HANDLE) == sizeof(Event)
		DWORD res = WaitForMultipleObjects(size, (::HANDLE *)&eventsArray[0], TRUE, milliseconds);
		return (res == WAIT_OBJECT_0);
	}

	


	class Guard;

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

		friend class MT::Guard;
	};


	class Guard
	{
		MT::CriticalSection & criticalSection;


		Guard( const Guard & ) : criticalSection(*((MT::CriticalSection*)nullptr)) {}
		Guard& operator=( const Guard &) {}


	public:

		Guard(MT::CriticalSection & cs) : criticalSection(cs)
		{
			criticalSection.Enter();
		}

		~Guard()
		{
			criticalSection.Leave();
		}
	};

	

}







