#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "process_group.h"
#include "list.h"

int init_process_monitor(struct process_monitor *monitor, int target_pid, int ignore_children)
{
	//hashtable initialization
	memset(&monitor->proctable, 0, sizeof(monitor->proctable));
	monitor->target_pid = target_pid;
	monitor->ignore_children = ignore_children;
	return 0;
}

struct list* get_process_list(struct process_monitor *monitor)
{
	struct process_iterator it;
	struct process process;
	init_process_iterator(&it);
	while (read_next_process(&it, &process) != -1)
	{
		if (monitor->ignore_children && process.pid != monitor->target_pid) continue;
		printf("Read process %d\n", process.pid);
		printf("Parent %d\n", process.ppid);
		printf("Starttime %d\n", process.starttime);
		printf("CPU time %d\n", process.cputime);

		int hashkey = pid_hashfn(process.pid + process.starttime);
		if (monitor->proctable[hashkey] == NULL)
		{

			if (is_ancestor(pid, ppid))
			struct process *ancestor = NULL;
			while((ancestor=seek_process(f, ppid))==NULL) {
				ppid = getppid_of(ppid);
			}


			//empty bucket
			monitor->proctable[hashkey] = malloc(sizeof(struct list));
			struct process *new_process = malloc(sizeof(struct process));
			memcpy(new_process, &process, sizeof(struct process));
			init_list(monitor->proctable[hashkey], 4);
			add_elem(monitor->proctable[hashkey], new_process);
		}
		else
		{

		}
	}
	close_process_iterator(&it);

	return NULL;
}

int close_process_monitor(struct process_monitor *monitor)
{
	int i;
	int size = sizeof(monitor->proctable) / sizeof(struct process*);
	for (i=0; i<size; i++) {
		if (monitor->proctable[i] != NULL) {
			//free() history for each process
			destroy_list(monitor->proctable[i]);
			free(monitor->proctable[i]);
			monitor->proctable[i] = NULL;
		}
	}
	monitor->target_pid = 0;
	return 0;
}