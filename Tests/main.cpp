#ifdef _WIN32

#include <stdlib.h>
#include <crtdbg.h>
#include <windows.h>

#pragma comment( lib, "winmm.lib" )

#endif

#include <stdio.h>
#include <string>
#include <set>

#include <MTScheduler.h>
#include "Tests/Tests.h"

#ifndef _WIN32

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
	}

	void *callStack[32];
  size_t size = backtrace(callStack, 32);

	printf("Error: signal %s:\n", name);

  char** symbollist = backtrace_symbols( callStack, size );

   // print the stack trace.
  for ( size_t i = 0; i < size; i++ )
  {
      printf("[%d, %lu] %s\n", i, (unsigned long int)currentThread, symbollist[i]);
	}

  free(symbollist);

  exit(1);
}
#endif

int main(int argc, char ** argv)
{

#ifdef _WIN32
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
	}

#ifdef _WIN32
	 timeEndPeriod(1);
#endif

	return res;
}



