#ifdef _WIN32

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#endif

#include <stdio.h>
#include <string>
#include <set>

#include "Platform.h"
#include "Scheduler.h"
#include "Tests/Tests.h"


int main(int argc, char **argv)
{
	argc;	argv;

#ifdef _WIN32
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG );
#endif

	Tests::RunAll();

#ifdef _WIN32
	 _CrtDumpMemoryLeaks();
#endif

	return 0;
}



