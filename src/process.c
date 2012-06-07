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

//TODO: add documentation to public functions

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include <sys/utsname.h>

#include "process.h"

#ifdef __APPLE__
#include <sys/sysctl.h>
#include <errno.h>
#endif

#ifdef __FreeBSD__
#include <limits.h>
#include <fcntl.h>
#include <kvm.h>
#include <paths.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#endif

#ifdef __linux__
int get_proc_info(struct process *p, pid_t pid) {
/*	static char statfile[20];
	static char buffer[64];
	int ret;
	sprintf(statfile, "/proc/%d/stat", pid);
	FILE *fd = fopen(statfile, "r");
	if (fd==NULL) return -1;
	fgets(buffer, sizeof(buffer), fd);
	fclose(fd);

	char state;

    int n = sscanf(buffer, "%d %s %c %d %d %d %d %d "
		"%lu %lu %lu %lu %lu %lu %lu "
		"%ld %ld %ld %ld %ld %ld "
		"%lu ",
		&p->pid,
		&p->command,
		&state,
		NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
		&utime,&stime,&cutime,&cstime,
		NULL,NULL,NULL,NULL,
		&starttime,
	);*/
	return 0;
}
#elif defined __APPLE__
int get_proc_info(struct process *p, pid_t pid) {
	int err;
	struct kinfo_proc *result = NULL;
	size_t length;
	int mib[] = {CTL_KERN, KERN_PROC, KERN_PROC_PID, pid};

	/* We start by calling sysctl with result == NULL and length == 0.
	   That will succeed, and set length to the appropriate length.
	   We then allocate a buffer of that size and call sysctl again
	   with that buffer.
	*/
	length = 0;
	err = sysctl(mib, 4, NULL, &length, NULL, 0);
	if (err == -1) {
		err = errno;
	}
	if (err == 0) {
		result = malloc(length);
		err = sysctl(mib, 4, result, &length, NULL, 0);
		if (err == -1)
			err = errno;
		if (err == ENOMEM) {
			free(result); /* clean up */
			result = NULL;
		}
	}

	p->pid = result->kp_proc.p_pid;
	p->ppid = result->kp_eproc.e_ppid;
	p->starttime = result->kp_proc.p_starttime.tv_sec;
	p->last_jiffies = result->kp_proc.p_cpticks;
	//p_pctcpu

	return 0;
}
#endif

// returns the start time of a process (used with pid to identify a process)
static int get_starttime(pid_t pid)
{
#ifdef __linux__
	char file[20];
	char buffer[1024];
	sprintf(file, "/proc/%d/stat", pid);
	FILE *fd = fopen(file, "r");
		if (fd==NULL) return -1;
	if (fgets(buffer, sizeof(buffer), fd) == NULL) return -1;
	fclose(fd);
	char *p = buffer;
	p = memchr(p+1,')', sizeof(buffer) - (p-buffer));
	int sp = 20;
	while (sp--)
		p = memchr(p+1,' ',sizeof(buffer) - (p-buffer));
	//start time of the process
	int time = atoi(p+1);
	return time;
#elif defined __APPLE__
	struct process proc;
	get_proc_info(&proc, pid);
	return proc.starttime;
#elif defined __FreeBSD__
	return -1;
#endif
}

static int get_jiffies(struct process *proc) {
#ifdef __linux__
	FILE *f = fopen(proc->stat_file, "r");
	if (f==NULL) return -1;
	if (fgets(proc->buffer, sizeof(proc->buffer),f) == NULL) return -1;
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
#elif defined __APPLE__
	struct process proc2;
	get_proc_info(&proc2, proc->pid);
	return proc2.last_jiffies;
#elif defined __FreeBSD__
   kvm_t *my_kernel = NULL;
   struct kinfo_proc *process_data = NULL;
   int processes;
   int my_jiffies = -1;
   my_kernel = kvm_open(0, 0, 0, O_RDONLY, "kvm_open");
   if (! my_kernel)
   {
      printf("Error opening kernel vm. You should be running as root.\n");
      return -1;
   }

   process_data = kvm_getprocs(my_kernel, KERN_PROC_PID, proc->pid, &processes);
   if ( (process_data) && (processes >= 1) )
       my_jiffies = process_data->ki_runtime;
   
   kvm_close(my_kernel);
   if (my_jiffies >= 0)
     my_jiffies /= 1000;
   return my_jiffies;
#endif
}

//return t1-t2 in microseconds (no overflow checks, so better watch out!)
static inline unsigned long timediff(const struct timeval *t1,const struct timeval *t2)
{
	return (t1->tv_sec - t2->tv_sec) * 1000000 + (t1->tv_usec - t2->tv_usec);
}

/*static int*/ int process_update(struct process *proc) {
	//TODO: get any process statistic here
	//check that starttime is not changed(?), update jiffies, parent, zombie status
	return 0;
}

int process_init(struct process *proc, int pid)
{
	//general members
	proc->pid = pid;
	proc->starttime = get_starttime(pid);
	proc->cpu_usage = 0;
	memset(&(proc->last_sample), 0, sizeof(struct timeval));
	proc->last_jiffies = -1;
	//system dependent members
#ifdef __linux__
//TODO: delete these members for the sake of portability?
	//test /proc file descriptor for reading
	sprintf(proc->stat_file, "/proc/%d/stat", pid);
	FILE *fd = fopen(proc->stat_file, "r");
	if (fd == NULL) return 1;
	fclose(fd);
#endif
	return 0;
}

//parameter in range 0-1
#define ALFA 0.08

int process_monitor(struct process *proc)
{
	int j = get_jiffies(proc);
	if (j<0) return -1; //error retrieving jiffies count (maybe the process is dead)
	struct timeval now;
	gettimeofday(&now, NULL);
	if (proc->last_jiffies==-1) {
		//store current time
		proc->last_sample = now;
		//store current jiffies
		proc->last_jiffies = j;
		//it's the first sample, cannot figure out the cpu usage
		proc->cpu_usage = -1;
		return 0;
	}
	//time from previous sample (in ns)
	long dt = timediff(&now, &(proc->last_sample));
	//how many jiffies in dt?
	double max_jiffies = dt * HZ / 1000000.0;
	double sample = (j - proc->last_jiffies) / max_jiffies;
	if (proc->cpu_usage == -1) {
		//initialization
		proc->cpu_usage = sample;
	}
	else {
		//usage adjustment
		proc->cpu_usage = (1-ALFA) * proc->cpu_usage + ALFA * sample;
	}

	//store current time
	proc->last_sample = now;
	//store current jiffies
	proc->last_jiffies = j;
	
	return 0;
}

int process_close(struct process *proc)
{
	if (kill(proc->pid,SIGCONT)!=0) {
		fprintf(stderr,"Process %d is already dead!\n", proc->pid);
	}
	proc->pid = 0;
	return 0;
}
