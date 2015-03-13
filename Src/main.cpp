#include <stdio.h>
#include <string>
#include <set>

#include "Platform.h"
#include "Scheduler.h"
#include "Tests/Tests.h"



void MT_CALL_CONV SubTaskEntryPoint(MT::FiberContext&, void* userData)
{
	int v = (int)userData;

	printf ("_SB = %d\n", v);

	Sleep(1000);

	printf ("_SE = %d\n", v);
}

void MT_CALL_CONV TaskEntryPoint(MT::FiberContext & context, void* userData)
{
	int v = (int)userData;

	printf ("B = %d\n", v);

	Sleep(1000);


	MT::TaskDesc tasks[2];
	tasks[0] = MT::TaskDesc(SubTaskEntryPoint, (void*)10);
	tasks[1] = MT::TaskDesc(SubTaskEntryPoint, (void*)11);

	context.RunSubtasks(&tasks[0], ARRAY_SIZE(tasks));

	printf ("E = %d\n", v);
}

int main(int argc, char **argv)
{
	printf("started\n");

	Tests::RunAll();

	argc;	argv;

	MT::TaskScheduler taskScheduler;

	MT::TaskDesc tasks[4];
	tasks[0] = MT::TaskDesc(TaskEntryPoint, (void*)0);
	tasks[1] = MT::TaskDesc(TaskEntryPoint, (void*)1);
	tasks[2] = MT::TaskDesc(TaskEntryPoint, (void*)5);
	tasks[3] = MT::TaskDesc(TaskEntryPoint, (void*)6);

	taskScheduler.RunTasks(MT::TaskGroup::GROUP_0, &tasks[0], ARRAY_SIZE(tasks));

	for(;;)
	{
		printf("Wait until all tasks is finished\n");

		bool isDone = taskScheduler.WaitGroup(MT::TaskGroup::GROUP_0, 2000);
		if (isDone)
		{
			printf("All tasks finished\n");
			break;
		}
	}


	return 0;
}



