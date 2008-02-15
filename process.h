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

#ifndef __PROCESS_H

#define __PROCESS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

//kernel time resolution (inverse of one jiffy interval) in Hertz
#define HZ sysconf(_SC_CLK_TCK)
//size of the history circular queue
#define HISTORY_SIZE 10

//extracted process statistics
struct cpu_usage {
	float pcpu;
	float workingrate;
};

//process instant photo
struct process_screenshot {
	//timestamp
	struct timespec when;
	//total jiffies used by the process
	int jiffies;
	//cpu time used since previous screenshot expressed in microseconds
	int cputime;
};

//process descriptor
struct process_history {
	//the PID of the process
	int pid;
	//name of /proc/PID/stat file
	char stat_file[20];
	//read buffer for /proc filesystem
	char buffer[1024];
	//recent history of the process (fixed circular queue)
	struct process_screenshot history[HISTORY_SIZE];
	//the index of the last screenshot made to the process
	int front_index;
	//the index of the oldest screenshot made to the process
	int tail_index;
	//how many screenshots we have in the queue (max HISTORY_SIZE)
	int actual_history_size;
	//total cputime saved in the history
	long total_workingtime;
	//current usage
	struct cpu_usage usage;
};

int process_init(struct process_history *proc, int pid);

int process_monitor(struct process_history *proc, int last_working_quantum, struct cpu_usage *usage);

int process_close(struct process_history *proc);

#endif
