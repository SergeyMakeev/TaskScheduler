#pragma once

#include <stdio.h>


#if defined(_MSC_VER)

inline void ThrowException()
{
	__debugbreak();
}

#else

#include<signal.h>

inline void ThrowException()
{
	raise(SIGTRAP);
}

#endif




#define REPORT_ASSERT( condition, description, file, line ) printf("%s. %s, line %d\n", description, file, line);ThrowException();

#define ASSERT( condition, description ) { if ( !(condition) ) { REPORT_ASSERT( #condition, description, __FILE__, __LINE__ ) } }
#define VERIFY( condition, description, operation ) { if ( !(condition) ) { { REPORT_ASSERT( #condition, description, __FILE__, __LINE__ ) }; operation; } }
