#pragma once

#include "types.h"
#include "Debug.h"

#define MT_CALL_CONV __stdcall

#define ARRAY_SIZE( arr ) ( sizeof( arr ) / sizeof( (arr)[0] ) )


namespace MT
{

	namespace EventReset
	{
		enum Type
		{
			AUTOMATIC = 0,
			MANUAL = 1,
		};
	}

}



#ifdef _WIN32
	#include "PlatformWindows.h"
#else
#include "PlatformPosix.h"
#endif


namespace MT
{

	//
	//
	//
	class ScopedGuard
	{
		MT::Mutex & mutex;

		ScopedGuard( const ScopedGuard & ) : mutex(*((MT::Mutex*)nullptr)) {}
		ScopedGuard& operator=( const ScopedGuard &) {}

	public:

		ScopedGuard(MT::Mutex & _mutex) : mutex(_mutex)
		{
			mutex.Lock();
		}

		~ScopedGuard()
		{
			mutex.Unlock();
		}
	};
}


