#pragma once


namespace MT
{
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


}


