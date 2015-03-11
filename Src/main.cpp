#include <stdio.h>
#include <string>
#include <set>

#include "Platform.h"
#include "TaskManager.h"


void MT_CALL_CONV TaskEntryPoint(void* userData)
{
	userData;
}

int main(int argc, char **argv)
{
	argc;	argv;

	MT::TaskManager taskManager;

	MT::TaskDesc tasks[4];
	tasks[0] = MT::TaskDesc(TaskEntryPoint, (void*)3);
	tasks[1] = MT::TaskDesc(TaskEntryPoint, (void*)4);
	tasks[2] = MT::TaskDesc(TaskEntryPoint, (void*)5);
	tasks[3] = MT::TaskDesc(TaskEntryPoint, (void*)6);

	taskManager.RunTasks(tasks);

	for(;;)
	{

	}

	return 0;
}



