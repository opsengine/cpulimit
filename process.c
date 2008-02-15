/**
 *
 * cpulimit - a cpu limiter for Linux
 *
 * Copyright (C) 2005-2008, by:  Angelo Marletta <marlonx80@hotmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "process.h"

int process_init(struct process_history *proc, int pid)
{
	proc->pid = pid;
	//init circular queue
	proc->front_index = -1;
	proc->tail_index = 0;
	proc->actual_history_size = 0;
	memset(proc->history, 0, sizeof(proc->history));
	proc->total_workingtime = 0;
	//test /proc file descriptor for reading
	sprintf(proc->stat_file, "/proc/%d/stat", proc->pid);
	FILE *fd = fopen(proc->stat_file, "r");
	fclose(fd);
	proc->usage.pcpu = 0;
	proc->usage.workingrate = 0;
	return (fd == NULL);
}

//return t1-t2 in microseconds (no overflow checks, so better watch out!)
static inline unsigned long timediff(const struct timespec *t1,const struct timespec *t2)
{
	return (t1->tv_sec - t2->tv_sec) * 1000000 + (t1->tv_nsec/1000 - t2->tv_nsec/1000);
}

static int get_jiffies(struct process_history *proc) {
	FILE *f = fopen(proc->stat_file, "r");
	if (f==NULL) return -1;
	fgets(proc->buffer, sizeof(proc->buffer),f);
	fclose(f);
	char *p = proc->buffer;
	p = memchr(p+1,')', sizeof(proc->buffer) - (p-proc->buffer));
	int sp = 12;
	while (sp--)
		p = memchr(p+1,' ',sizeof(proc->buffer) - (p-proc->buffer));
	//user mode jiffies
	int utime = atoi(p+1);
	p = memchr(p+1,' ',sizeof(proc->buffer) - (p-proc->buffer));
	//kernel mode jiffies
	int ktime = atoi(p+1);
	return utime+ktime;
}

int process_monitor(struct process_history *proc, int last_working_quantum, struct cpu_usage *usage)
{
	//increment front index
	proc->front_index = (proc->front_index+1) % HISTORY_SIZE;

	//actual history size is: (front-tail+HISTORY_SIZE)%HISTORY_SIZE+1
	proc->actual_history_size = (proc->front_index - proc->tail_index + HISTORY_SIZE) % HISTORY_SIZE + 1;

	//actual front and tail of the queue
	struct process_screenshot *front = &(proc->history[proc->front_index]);
	struct process_screenshot *tail = &(proc->history[proc->tail_index]);

	//take a shot and save in front
	int j = get_jiffies(proc);
	if (j<0) return -1; //error retrieving jiffies count (maybe the process is dead)
	front->jiffies = j;
	clock_gettime(CLOCK_REALTIME, &(front->when));
	front->cputime = last_working_quantum;

	if (proc->actual_history_size==1) {
		//not enough elements taken (it's the first one!), return 0
		usage->pcpu = -1;
		usage->workingrate = 1;
		return 0;
	}
	else {
		//queue has enough elements
		//now we can calculate cpu usage, interval dt and dtwork are expressed in microseconds
		long dt = timediff(&(front->when), &(tail->when));
		//the total time between tail and front in which the process was allowed to run
		proc->total_workingtime += front->cputime - tail->cputime;
		int used_jiffies = front->jiffies - tail->jiffies;
		float usage_ratio = (used_jiffies*1000000.0/HZ) / proc->total_workingtime;
		usage->workingrate = 1.0 * proc->total_workingtime / dt;
		usage->pcpu = usage_ratio * usage->workingrate;
		//increment tail index if the queue is full
		if (proc->actual_history_size==HISTORY_SIZE)
			proc->tail_index = (proc->tail_index+1) % HISTORY_SIZE;
		return 0;
	}
}

int process_close(struct process_history *proc)
{
	if (kill(proc->pid,SIGCONT)!=0) {
		fprintf(stderr,"Process %d is already dead!\n", proc->pid);
	}
	proc->pid = 0;
	return 0;
}
