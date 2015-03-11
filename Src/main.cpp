#include <stdio.h>
#include <string>
#include <set>

#include "Platform.h"
#include "TaskManager.h"


void MT_CALL_CONV TaskEntryPoint(void* userData)
{
	Sleep(3000);
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

	taskManager.RunTasks(MT::TaskGroup::GROUP_0, tasks);

	for(;;)
	{
		printf("Wait until all tasks is finished\n");

		bool isDone = taskManager.WaitGroup(MT::TaskGroup::GROUP_0, 2000);
		if (isDone)
		{
			printf("All tasks finished\n");
			break;
		}
	}


	return 0;
}



