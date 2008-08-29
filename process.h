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

//kernel time resolution timer interrupt frequency in Hertz
#define HZ sysconf(_SC_CLK_TCK)

//process descriptor
struct process_history {
	//the PID of the process
	pid_t pid;
#ifdef __GNUC__
	//name of /proc/PID/stat file
	char stat_file[20];
	//read buffer for /proc filesystem
	char buffer[1024];
#endif
	//timestamp when last_j and cpu_usage was calculated
	struct timeval last_sample;
	//total number of jiffies used by the process at time last_sample
	int last_jiffies;
	//cpu usage estimation (value in range 0-1)
	double cpu_usage;
};

int process_init(struct process_history *proc, pid_t pid);

int process_monitor(struct process_history *proc);

int process_close(struct process_history *proc);

#endif
