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
 *
 **********************************************************************
 *
 * This is a simple program to limit the cpu usage of a process
 * If you modify this code, send me a copy please
 *
 * Date:    15/2/2008
 * Version: 1.2 alpha
 * Get the latest version at: http://cpulimit.sourceforge.net
 *
 * Changelog:
 * - reorganization of the code, splitted in more source files
 * - control function process_monitor() optimized by eliminating an unnecessary loop
 * - experimental support for multiple control of children processes and threads
 *   children detection algorithm seems heavy because of the amount of code,
 *   but it's designed to be scalable when there are a lot of children processes
 * - cpu count detection, i.e. if you have 4 cpu, it is possible to limit up to 400%
 * - in order to avoid deadlock, cpulimit prevents to limit itself
 * - option --path eliminated, use --exe instead both for absolute path and file name
 * - deleted almost every setpriority(), just set it once at startup
 * - minor enhancements and bugfixes
 *
 */


#include <getopt.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/resource.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>

#include "process.h"
#include "procutils.h"
#include "list.h"

//some useful macro
#define MIN(a,b) (a<b?a:b)
#define MAX(a,b) (a>b?a:b)
#define print_caption()	printf("\n%%CPU\twork quantum\tsleep quantum\tactive rate\n")

//control time slot in microseconds
//each slot is splitted in a working slice and a sleeping slice
#define TIME_SLOT 100000

#define MAX_PRIORITY -10

/* GLOBAL VARIABLES */

//the "family"
struct process_family pf;
//pid of cpulimit
int cpulimit_pid;
//name of this program (maybe cpulimit...)
char *program_name;

/* CONFIGURATION VARIABLES */

//verbose mode
int verbose = 0;
//lazy mode (exits if there is no process)
int lazy = 0;

//how many cpu do we have?
int get_cpu_count()
{
	FILE *fd;
	int cpu_count = 0;
	char line[100];
	fd = fopen("/proc/stat", "r");
	if (fd < 0)
		return 0; //are we running Linux??
	while (fgets(line,sizeof(line),fd)!=NULL) {
		if (strncmp(line, "cpu", 3) != 0) break;
		cpu_count++;
	}
	fclose(fd);
	return cpu_count - 1;
}

//return t1-t2 in microseconds (no overflow checks, so better watch out!)
inline unsigned long timediff(const struct timespec *t1,const struct timespec *t2)
{
	return (t1->tv_sec - t2->tv_sec) * 1000000 + (t1->tv_nsec/1000 - t2->tv_nsec/1000);
}

//returns t1-t2 in microseconds
inline unsigned long long tv_diff(struct timeval *t1, struct timeval *t2)
{
	return ((unsigned long long)(t1->tv_sec - t2->tv_sec)) * 1000000ULL + t1->tv_usec - t2->tv_usec;
}

//SIGINT and SIGTERM signal handler
void quit(int sig)
{
	//let all the processes continue if stopped
	struct list_node *node = NULL;
	for (node=pf.members.first; node!= NULL; node=node->next) {
		struct process *p = (struct process*)(node->data);
		process_close(p->history);
		kill(p->pid, SIGCONT);
	}
	//free all the memory
	cleanup_process_family(&pf);
	exit(0);
}

void print_usage(FILE *stream, int exit_code)
{
	fprintf(stream, "Usage: %s TARGET [OPTIONS...]\n",program_name);
	fprintf(stream, "   TARGET must be exactly one of these:\n");
	fprintf(stream, "      -p, --pid=N        pid of the process (implies -z)\n");
	fprintf(stream, "      -e, --exe=FILE     name of the executable program file or absolute path name\n");
	fprintf(stream, "   OPTIONS\n");
	fprintf(stream, "      -l, --limit=N      percentage of cpu allowed from 0 to 100 (required)\n");
	fprintf(stream, "      -v, --verbose      show control statistics\n");
	fprintf(stream, "      -z, --lazy         exit if there is no suitable target process, or if it dies\n");
	fprintf(stream, "      -h, --help         display this help and exit\n");
	exit(exit_code);
}

void limit_process(int pid, float limit)
{
	//slice of the slot in which the process is allowed to run
	struct timespec twork;
	//slice of the slot in which the process is stopped
	struct timespec tsleep;
	//when the last twork has started
	struct timespec startwork;
	//when the last twork has finished
	struct timespec endwork;
	//initialization
	memset(&twork, 0, sizeof(struct timespec));
	memset(&tsleep, 0, sizeof(struct timespec));
	memset(&startwork, 0, sizeof(struct timespec));
	memset(&endwork, 0, sizeof(struct timespec));	
	//last working time in microseconds
	unsigned long workingtime = 0;
	int i = 0;

	//build the family
	create_process_family(&pf, pid);
	struct list_node *node;
	
	if (verbose) printf("Members in the family owned by %d: %d\n", pf.father, pf.members.count);

	while(1) {

		if (i%100==0 && verbose) print_caption();

		if (i%10==0) {
			//update the process family (checks only for new members)
			int new_children = update_process_family(&pf);
			if (new_children) {
				printf("%d new children processes detected (", new_children);
				int j;
				node = pf.members.last;
				for (j=0; j<new_children; j++) {
					printf("%d", ((struct process*)(node->data))->pid);
					if (j<new_children-1) printf(" ");
					node = node->previous;
				}
				printf(")\n");
			}
		}

		//total cpu actual usage (range 0-1)
		//1 means that the processes are using 100% cpu
		float pcpu = 0;
		//rate at which we are keeping active the processes (range 0-1)
		//1 means that the process are using all the twork slice
		float workingrate = 0;

		//estimate how much the controlled processes are using the cpu in the working interval
		for (node=pf.members.first; node!=NULL; node=node->next) {
			struct process *proc = (struct process*)(node->data);
			if (process_monitor(proc->history, workingtime, &(proc->history->usage))==-1) {
				//process is dead, remove it from family
				fprintf(stderr,"Process %d dead!\n", proc->pid);
				remove_process_from_family(&pf, proc->pid);
				continue;
			}
			pcpu += proc->history->usage.pcpu;
			workingrate += proc->history->usage.workingrate;
		}
		//average value
		workingrate /= pf.members.count;

		//TODO: make workingtime customized for each process, now it's equal for all

		//adjust work and sleep time slices
		if (pcpu>0) {
			twork.tv_nsec = MIN(TIME_SLOT*limit*1000/pcpu*workingrate,TIME_SLOT*1000);
		}
		else if (pcpu==0) {
			twork.tv_nsec = TIME_SLOT*1000;
		}
		else if (pcpu==-1) {
			//not yet a valid idea of cpu usage
			pcpu = limit;
			workingrate = limit;
			twork.tv_nsec = MIN(TIME_SLOT*limit*1000,TIME_SLOT*1000);
		}
		tsleep.tv_nsec = TIME_SLOT*1000-twork.tv_nsec;

		if (verbose && i%10==0 && i>0) {
			printf("%0.2f%%\t%6ld us\t%6ld us\t%0.2f%%\n",pcpu*100,twork.tv_nsec/1000,tsleep.tv_nsec/1000,workingrate*100);
		}

		//resume processes
		for (node=pf.members.first; node!=NULL; node=node->next) {
			struct process *proc = (struct process*)(node->data);
			if (kill(proc->pid,SIGCONT)!=0) {
				//process is dead, remove it from family
				fprintf(stderr,"Process %d dead!\n", proc->pid);
				remove_process_from_family(&pf, proc->pid);
			}
		}

		//now processes are free to run (same working slice for all)
		clock_gettime(CLOCK_REALTIME,&startwork);
		nanosleep(&twork,NULL);
		clock_gettime(CLOCK_REALTIME,&endwork);
		workingtime = timediff(&endwork,&startwork);

		if (tsleep.tv_nsec>0) {
			//stop only if the process is active
			//stop processes, they have worked enough
			for (node=pf.members.first; node!=NULL; node=node->next) {
				struct process *proc = (struct process*)(node->data);
				if (kill(proc->pid,SIGSTOP)!=0) {
					//process is dead, remove it from family
					fprintf(stderr,"Process %d dead!\n", proc->pid);
					remove_process_from_family(&pf, proc->pid);
				}
			}
			//now the processes are sleeping
			nanosleep(&tsleep,NULL);
		}
		i++;
	}
	cleanup_process_family(&pf);
}

int main(int argc, char **argv) {

	//get program name
	char *p=(char*)memrchr(argv[0],(unsigned int)'/',strlen(argv[0]));
	program_name = p==NULL?argv[0]:(p+1);
	cpulimit_pid = getpid();

	//argument variables
	const char *exe = NULL;
	int perclimit = 0;
	int pid_ok = 0;
	int process_ok = 0;
	int limit_ok = 0;
	int pid = 0;

	//parse arguments
	int next_option;
    int option_index = 0;
	//A string listing valid short options letters
	const char* short_options = "p:e:l:vzh";
	//An array describing valid long options
	const struct option long_options[] = {
		{ "pid",        required_argument, NULL,     'p' },
		{ "exe",        required_argument, NULL,     'e' },
		{ "limit",      required_argument, NULL,     'l' },
		{ "verbose",    no_argument,       &verbose, 'v' },
		{ "lazy",       no_argument,       &lazy,    'z' },
		{ "help",       no_argument,       NULL,     'h' },
		{ 0,            0,                 0,         0  }
	};

	do {
		next_option = getopt_long(argc, argv, short_options,long_options, &option_index);
		switch(next_option) {
			case 'p':
				pid = atoi(optarg);
				//todo: verify pid is valid
				pid_ok = 1;
				process_ok = 1;
				break;
			case 'e':
				exe = optarg;
				process_ok = 1;
				break;
			case 'l':
				perclimit = atoi(optarg);
				limit_ok = 1;
				break;
			case 'v':
				verbose = 1;
				break;
			case 'z':
				lazy = 1;
				break;
			case 'h':
				print_usage(stdout, 1);
				break;
			case '?':
				print_usage(stderr, 1);
				break;
			case -1:
				break;
			default:
				abort();
		}
	} while(next_option != -1);

	if (pid!=0) {
		lazy = 1;
	}
	
	if (pid_ok && (pid<=1 || pid>=65536)) {
		fprintf(stderr,"Error: Invalid value for argument PID\n");
		print_usage(stderr, 1);
		exit(1);
	}
	
	if (!process_ok) {
		fprintf(stderr,"Error: You must specify a target process, by name or by PID\n");
		print_usage(stderr, 1);
		exit(1);
	}
	if (pid_ok && exe!=NULL) {
		fprintf(stderr, "Error: You must specify exactly one process, by name or by PID\n");
		print_usage(stderr, 1);
		exit(1);
	}
	if (!limit_ok) {
		fprintf(stderr,"Error: You must specify a cpu limit percentage\n");
		print_usage(stderr, 1);
		exit(1);
	}
	float limit = perclimit/100.0;
	int cpu_count = get_cpu_count();
	if (limit<0 || limit >cpu_count) {
		fprintf(stderr,"Error: limit must be in the range 0-%d00\n", cpu_count);
		print_usage(stderr, 1);
		exit(1);
	}
	//parameters are all ok!
	signal(SIGINT, quit);
	signal(SIGTERM, quit);

	//try to renice with the best value
	int old_priority = getpriority(PRIO_PROCESS, 0);
	int priority = old_priority;
	while (setpriority(PRIO_PROCESS, 0, priority-1) == 0 && priority>MAX_PRIORITY) {
		priority--;	
	}
	if (priority != old_priority) {
		printf("Priority changed to %d\n", priority);
	}
	else {
		printf("Cannot change priority\n");
	}
	
	while(1) {
		//look for the target process..or wait for it
		int ret = 0;
		if (pid_ok) {
			//search by pid
			ret = look_for_process_by_pid(pid);
			if (ret == 0) {
				printf("No process found\n");
			}
			else if (ret < 0) {
				printf("Process found but you aren't allowed to control it\n");
			}
		}
		else {
			//search by file or path name
			ret = look_for_process_by_name(exe);
			if (ret == 0) {
				printf("No process found\n");
			}
			else if (ret < 0) {
				printf("Process found but you aren't allowed to control it\n");
			}
			else {
				pid = ret;
			}
		}
		if (ret > 0) {
			if (ret == cpulimit_pid) {
				printf("Process %d is cpulimit itself! Aborting to avoid deadlock\n", ret);
				exit(1);
			}
			printf("Process %d found\n", pid);
			//control
			limit_process(pid, limit);
		}
		if (lazy) {
			printf("Giving up...\n");
			break;
		}
		sleep(2);
	}
	
	return 0;
}

