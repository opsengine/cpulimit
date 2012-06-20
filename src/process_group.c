#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include "process_iterator.h"
#include "process_group.h"
#include "list.h"

int init_process_group(struct process_group *pgroup, int target_pid, int include_children)
{
	//hashtable initialization
	memset(&pgroup->proctable, 0, sizeof(pgroup->proctable));
	pgroup->target_pid = target_pid;
	pgroup->include_children = include_children;
	pgroup->proclist = (struct list*)malloc(sizeof(struct list));
	return 0;
}

int close_process_group(struct process_group *pgroup)
{
	int i;
	int size = sizeof(pgroup->proctable) / sizeof(struct process*);
	for (i=0; i<size; i++) {
		if (pgroup->proctable[i] != NULL) {
			//free() history for each process
			destroy_list(pgroup->proctable[i]);
			free(pgroup->proctable[i]);
			pgroup->proctable[i] = NULL;
		}
	}
	free(pgroup->proclist);
	pgroup->proclist = NULL;
	return 0;
}

void update_process_group(struct process_group *pgroup)
{
	struct process_iterator it;
	struct process process;
	struct process_filter filter;
	filter.pid = pgroup->target_pid;
	filter.include_children = pgroup->include_children;
	init_process_iterator(&it, &filter);
	while (get_next_process(&it, &process) != -1)
	{
		printf("Read process %d\n", process.pid);
		printf("Parent %d\n", process.ppid);
		printf("Starttime %d\n", process.starttime);
		printf("CPU time %d\n", process.cputime);
		int hashkey = pid_hashfn(process.pid + process.starttime);
		if (pgroup->proctable[hashkey] == NULL)
		{
			//empty bucket
			pgroup->proctable[hashkey] = malloc(sizeof(struct list));
			struct process *new_process = malloc(sizeof(struct process));
			memcpy(new_process, &process, sizeof(struct process));
			init_list(pgroup->proctable[hashkey], 4);
			add_elem(pgroup->proctable[hashkey], new_process);
		}
		else
		{
			//existing bucket
			
		}
	}
	close_process_iterator(&it);
}
