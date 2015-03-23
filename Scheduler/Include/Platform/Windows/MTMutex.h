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

	private:

		Mutex(const Mutex &) {}
		void operator=(const Mutex &) {}

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

	private:

		void Lock()
		{
			::EnterCriticalSection( &criticalSection );
		}
		void Unlock()
		{
			::LeaveCriticalSection( &criticalSection );
		}

	};


}


