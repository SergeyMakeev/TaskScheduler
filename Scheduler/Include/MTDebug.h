// The MIT License (MIT)
//
// 	Copyright (c) 2015 Sergey Makeev, Vadim Slyusarev
//
// 	Permission is hereby granted, free of charge, to any person obtaining a copy
// 	of this software and associated documentation files (the "Software"), to deal
// 	in the Software without restriction, including without limitation the rights
// 	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// 	copies of the Software, and to permit persons to whom the Software is
// 	furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
// 	all copies or substantial portions of the Software.
//
// 	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// 	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// 	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// 	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// 	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// 	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// 	THE SOFTWARE.

#pragma once

#include "MTTools.h"
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

	// force access violation error
	char* pBadAddr = (char*)0x0;
	*pBadAddr = 0;
}

#endif




#if defined(_DEBUG) || defined(MT_INSTRUMENTED_BUILD)

#define MT_REPORT_ASSERT_IMPL( condition, description, file, line ) printf("Assertion failed : %s. File %s, line %d. Condition %s\n", description, file, line, #condition); ThrowException();


#define MT_REPORT_ASSERT( description ) { MT_REPORT_ASSERT_IMPL( "always", description, __FILE__, __LINE__ ) }
#define MT_ASSERT( condition, description ) { if ( !(condition) ) { MT_REPORT_ASSERT_IMPL( #condition, description, __FILE__, __LINE__ ) } }
#define MT_VERIFY( condition, description, operation ) { if ( !(condition) ) { { MT_REPORT_ASSERT_IMPL( #condition, description, __FILE__, __LINE__ ) }; operation; } }

#else

//TODO: Remove condition from MT_ASSERT
//      Currently condition removal produces too many unused variable warnings when compiling
#define MT_REPORT_ASSERT( description )
#define MT_ASSERT( condition, description ) { if ( !(condition) ) {} }
#define MT_VERIFY( condition, description, operation ) { if ( !(condition) ) { operation; } }

#endif
