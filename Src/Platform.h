#pragma once

#include <time.h> 

#include "types.h"
#include "Debug.h"

typedef void (*TThreadEntryPoint)(void* userData);

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
	#include "Platform/Windows/Common.h"
#else
	#include "Platform/Posix/Common.h"
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

