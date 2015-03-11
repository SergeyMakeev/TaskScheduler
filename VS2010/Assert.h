#pragma once

#define REPORT_ASSERT( #condition, description ) { __debugbreak(); }

#define ASSERT( condition, description ) { if ( !(condition) ) { REPORT_ASSERT( #condition, description ); } }
#define VERIFY( condition, description, operation ) { if ( !(condition) ) { { REPORT_ASSERT( #condition, description ) }; operation; } }