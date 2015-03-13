#pragma once

// __debugbreak is msvc only, so throw null pointer exception
inline void ThrowException()
{
	__debugbreak();
	/*
	char * p = nullptr;
	p += 13;
	*p = 13;
	*/
};

#define REPORT_ASSERT( condition, description, file, line ) printf("%s. %s, line %d\n", description, file, line);ThrowException();

#define ASSERT( condition, description ) { if ( !(condition) ) { REPORT_ASSERT( #condition, description, __FILE__, __LINE__ ) } }
#define VERIFY( condition, description, operation ) { if ( !(condition) ) { { REPORT_ASSERT( #condition, description, __FILE__, __LINE__ ) }; operation; } }
