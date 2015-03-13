#pragma once

// __debugbreak is msvc only, so throw null pointer exception
inline void ThrowException()
{
	char * p = nullptr;
	*p = 3;
};

#define REPORT_ASSERT( condition, description ) ThrowException();

#define ASSERT( condition, description ) { if ( !(condition) ) { REPORT_ASSERT( #condition, description ) } }
#define VERIFY( condition, description, operation ) { if ( !(condition) ) { { REPORT_ASSERT( #condition, description ) }; operation; } }
