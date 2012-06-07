/**
 *
 * cpulimit - a cpu limiter for Linux
 *
 * Copyright (C) 2005-2012, by:  Angelo Marletta <marlonx80@hotmail.com>
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

#ifndef __PROCESS_ITERATOR_H

#define __PROCESS_ITERATOR_H

#include <unistd.h>
#include <limits.h>
#include <dirent.h>

#ifdef __FreeBSD__
#include <kvm.h>
#endif

// process descriptor
struct process {
	//pid of the process
	pid_t pid;
	//pid of the process
	pid_t ppid;
	//start time
	int starttime;
	//total number of jiffies used by the process at time last_sample
	int last_jiffies;
	//actual cpu usage estimation (value in range 0-1)
	double cpu_usage;
	//1 if the process is zombie
	int is_zombie;
	//absolute path of the executable file
	char command[PATH_MAX+1];
};

struct process_iterator {
#ifdef __linux__
	DIR *dip;
	struct dirent *dit;
#elif defined __APPLE__
	struct kinfo_proc *procList;
#elif defined __FreeBSD__
	kvm_t *kd;
	struct kinfo_proc *procs;
	int count;
	int i;
#endif
};

int init_process_iterator(struct process_iterator *i);

int read_next_process(struct process_iterator *i, struct process *p);

int close_process_iterator(struct process_iterator *i);

#endif
