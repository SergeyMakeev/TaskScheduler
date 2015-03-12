#pragma once



#if _MSC_VER

#define DEBUG_BREAK __debugbreak()

#elif __GNUC__

//#define DEBUG_BREAK raise(SIGTRAP)
#define DEBUG_BREAK

#else

#error "Platform not specified"

#endif



#define REPORT_ASSERT( condition, description ) DEBUG_BREAK;

#define ASSERT( condition, description ) { if ( !(condition) ) { REPORT_ASSERT( #condition, description ) } }
#define VERIFY( condition, description, operation ) { if ( !(condition) ) { { REPORT_ASSERT( #condition, description ) }; operation; } }
