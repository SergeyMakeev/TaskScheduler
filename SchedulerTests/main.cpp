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

#ifdef _WIN32

#include <stdlib.h>
#include <crtdbg.h>
#include <windows.h>


#if _MSC_VER > 1600
#include <timeapi.h>
#endif

#pragma comment( lib, "winmm.lib" )

#endif

#include <stdio.h>
#include <string>
#include <set>

#include <MTScheduler.h>
#include "Tests/Tests.h"

#if !defined(_WIN32) 

#include <execinfo.h>

void PosixSignalHandler(int signum)
{
	pthread_t currentThread = pthread_self();

	const char* name = "Unknown";
	switch( signum )
	{
		case SIGABRT: name = "SIGABRT";  break;
		case SIGSEGV: name = "SIGSEGV";  break;
		case SIGBUS:  name = "SIGBUS";   break;
		case SIGILL:  name = "SIGILL";   break;
		case SIGFPE:  name = "SIGFPE";   break;
        case SIGTRAP: name = "SIGTRAP";  break;
	}

	void *callStack[32];
  size_t size = backtrace(callStack, 32);

	printf("Error: signal %s:\n", name);

  char** symbollist = backtrace_symbols( callStack, size );

   // print the stack trace.
  for ( size_t i = 0; i < size; i++ )
  {
      printf("[%zu, %lu] %s\n", i, (unsigned long int)currentThread, symbollist[i]);
	}

  free(symbollist);

  exit(1);
}
#endif


int main(int argc, char ** argv)
{

#if defined(_WIN32)
	timeBeginPeriod(1);
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG );
#else
	// install signal handler
	signal(SIGSEGV, PosixSignalHandler);
	signal(SIGTRAP, PosixSignalHandler);
#endif

	int passCount = 1;
	if (argc >= 2)
	{
		passCount = atoi(argv[1]);
	}

	int res = 0;
	printf("Tests will run %d times\n", passCount);
	for(int pass = 0; pass < passCount; pass++)
	{
		printf("---- [ attempt #%d] ----\n", pass + 1);

		res = Tests::RunAll();
		if (res != 0)
		{
			printf("Unit test failed - pass %d of %d\n", pass + 1, passCount);
			return res;
		}
	}

#if defined(_WIN32)
	 timeEndPeriod(1);
#endif

	return res;
}



