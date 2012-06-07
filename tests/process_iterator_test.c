#include <stdio.h>
#include <process_iterator.h>

int main()
{
	struct process_iterator it;
	struct process process;
	init_process_iterator(&it);
	while (read_next_process(&it, &process) != -1)
	{
		printf("Read process %d\n", process.pid);
		printf("Parent %d\n", process.ppid);
//		printf("Starttime %d\n", process.starttime);
		printf("Jiffies %d\n", process.last_jiffies);
	}
	close_process_iterator(&it);
	return 0;
}
