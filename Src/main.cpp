#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include <stdio.h>
#include <string>
#include <set>

#include "Platform.h"
#include "Scheduler.h"
#include "Tests/Tests.h"


int main(int argc, char **argv)
{
	argc;	argv;
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG );

	Tests::RunAll();

	 _CrtDumpMemoryLeaks();

	return 0;
}



