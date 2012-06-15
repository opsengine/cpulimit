#include <stdio.h>
#include <process_iterator.h>

int main()
{
	struct process_iterator it;
	struct process process;
	struct process_filter filter;
	filter.pid = 2981;
	filter.include_children = 1;
	init_process_iterator(&it, &filter);
	while (read_next_process(&it, &process) == 0)
	{
		printf("Read process %d\n", process.pid);
		printf("Parent %d\n", process.ppid);
		printf("Starttime %d\n", process.starttime);
		printf("CPU time %d\n", process.cputime);
		printf("\n");
	}
	close_process_iterator(&it);
	return 0;
}
