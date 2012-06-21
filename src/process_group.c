#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/time.h>

#include <assert.h>

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
	init_list(pgroup->proclist, 4);
	memset(&pgroup->last_update, 0, sizeof(pgroup->last_update));
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
	clear_list(pgroup->proclist);
	free(pgroup->proclist);
	pgroup->proclist = NULL;
	return 0;
}

void remove_terminated_processes(struct process_group *pgroup)
{
	//TODO
}

//return t1-t2 in microseconds (no overflow checks, so better watch out!)
static inline unsigned long timediff(const struct timeval *t1,const struct timeval *t2)
{
	return (t1->tv_sec - t2->tv_sec) * 1000000 + (t1->tv_usec - t2->tv_usec);
}

//parameter in range 0-1
#define ALFA 0.08

int process_monitor(struct process *proc)
{
	// int j = get_jiffies(proc);
	// if (j<0) return -1; //error retrieving jiffies count (maybe the process is dead)
	// struct timeval now;
	// gettimeofday(&now, NULL);
	// if (proc->last_jiffies==-1) {
	// 	//store current time
	// 	proc->last_sample = now;
	// 	//store current jiffies
	// 	proc->last_jiffies = j;
	// 	//it's the first sample, cannot figure out the cpu usage
	// 	proc->cpu_usage = -1;
	// 	return 0;
	// }
	// //time from previous sample (in ns)
	// long dt = timediff(&now, &(proc->last_sample));
	// //how many jiffies in dt?
	// double max_jiffies = dt * HZ / 1000000.0;
	// double sample = (j - proc->last_jiffies) / max_jiffies;
	// if (proc->cpu_usage == -1) {
	// 	//initialization
	// 	proc->cpu_usage = sample;
	// }
	// else {
	// 	//usage adjustment
	// 	proc->cpu_usage = (1-ALFA) * proc->cpu_usage + ALFA * sample;
	// }
	// //store current time
	// proc->last_sample = now;
	// //store current jiffies
	// proc->last_jiffies = j;	
	return 0;
}

void update_process_group(struct process_group *pgroup)
{
	struct process_iterator it;
	struct process tmp_process;
	struct process_filter filter;
	filter.pid = pgroup->target_pid;
	filter.include_children = pgroup->include_children;
	init_process_iterator(&it, &filter);
	clear_list(pgroup->proclist);
	init_list(pgroup->proclist, 4);
	struct timeval now;
	gettimeofday(&now, NULL);
	//time elapsed from previous sample (in ms)
	long dt = timediff(&now, &pgroup->last_update) / 1000;
	while (get_next_process(&it, &tmp_process) != -1)
	{
		// printf("Read process %d\n", tmp_process.pid);
		// printf("Parent %d\n", tmp_process.ppid);
		// printf("Starttime %d\n", tmp_process.starttime);
		// printf("CPU time %d\n", tmp_process.cputime);
		int hashkey = pid_hashfn(tmp_process.pid + tmp_process.starttime);
		if (pgroup->proctable[hashkey] == NULL)
		{
			//empty bucket
			pgroup->proctable[hashkey] = malloc(sizeof(struct list));
			struct process *new_process = malloc(sizeof(struct process));
			tmp_process.cpu_usage = -1;
			memcpy(new_process, &tmp_process, sizeof(struct process));
			init_list(pgroup->proctable[hashkey], 4);
			add_elem(pgroup->proctable[hashkey], new_process);
			add_elem(pgroup->proclist, new_process);
		}
		else
		{
			//existing bucket
			struct process *p = (struct process*)locate_elem(pgroup->proctable[hashkey], &tmp_process);
			if (p == NULL)
			{
				//process is new. add it
				struct process *new_process = malloc(sizeof(struct process));
				tmp_process.cpu_usage = -1;
				memcpy(new_process, &tmp_process, sizeof(struct process));
				add_elem(pgroup->proctable[hashkey], new_process);
				add_elem(pgroup->proclist, new_process);
			}
			else
			{
				assert(tmp_process.pid == p->pid);
				assert(tmp_process.ppid == p->ppid);
				assert(tmp_process.starttime == p->starttime);
				//process exists. update CPU usage
				double sample = (tmp_process.cputime - p->cputime) / dt;
				if (p->cpu_usage == -1) {
					//initialization
					p->cpu_usage = sample;
				}
				else {
					//usage adjustment
					p->cpu_usage = (1-ALFA) * p->cpu_usage + ALFA * sample;
				}
				p->cpu_usage = (1-ALFA) * p->cpu_usage + ALFA * sample;
				p->cputime = tmp_process.cputime;
				add_elem(pgroup->proclist, p);
			}
		}
	}
	close_process_iterator(&it);
	pgroup->last_update = now;
}
