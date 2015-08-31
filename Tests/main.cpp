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

int main(int argc, char ** argv)
{

#ifdef _WIN32
	timeBeginPeriod(1);
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG );
#endif

	int passCount = 1;
	if (argc >= 2)
	{
		passCount = atoi(argv[1]);
	}

	int res = 0;
	printf("Tests run %d times\n", passCount);
	for(int pass = 0; pass < passCount; pass++)
	{
		res = Tests::RunAll();
	}

#ifdef _WIN32
	 timeEndPeriod(1);
#endif

	return res;
}



