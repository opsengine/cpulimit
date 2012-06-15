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

#ifndef __PROCESS_MONITOR_H

#define __PROCESS_MONITOR_H

#include "process_iterator.h"

#include "list.h"

#define PIDHASH_SZ 1024
#define pid_hashfn(x) ((((x) >> 8) ^ (x)) & (PIDHASH_SZ - 1))

struct process_monitor
{
	//hashtable with all the processes (array of struct list of struct process)
	struct list *proctable[PIDHASH_SZ];
	pid_t target_pid;
	int ignore_children;
};

int init_process_monitor(struct process_monitor *monitor, int target_pid, int ignore_children);

struct list* get_process_list(struct process_monitor *monitor);

int close_process_monitor(struct process_monitor *monitor);

#endif