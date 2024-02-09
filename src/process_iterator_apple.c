/**
 *
 * cpulimit - a CPU limiter for Linux
 *
 * Copyright (C) 2005-2012, by:  Angelo Marletta <angelo dot marletta at gmail dot com>
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
 * Author: Simon Sigurdhsson
 *
 */

#ifdef __APPLE__

#ifndef __PROCESS_ITERATOR_APPLE_C
#define __PROCESS_ITERATOR_APPLE_C

#include <errno.h>
#include <stdio.h>
#include <libproc.h>
#include <string.h>
#include <stdlib.h>

static int unique_nonzero_pids(pid_t *arr_in, int len_in, pid_t *arr_out)
{
	pid_t *source = arr_in;
	int len_out = 0;
	int i, j;
	if (arr_out == NULL)
		return -1;
	if (arr_in == arr_out)
	{
		source = malloc(sizeof(pid_t) * len_in);
		if (source == NULL)
		{
			exit(-1);
		}
		memcpy(source, arr_in, sizeof(pid_t) * len_in);
	}
	for (i = 0; i < len_in; i++)
	{
		int found = 0;
		if (source[i] == 0)
			continue;
		for (j = 0; !found && j < len_out; j++)
		{
			found = (source[i] == arr_out[j]);
		}
		if (!found)
		{
			arr_out[len_out++] = source[i];
		}
	}
	if (arr_in == arr_out)
	{
		free(source);
	}
	return len_out - 1;
}

int init_process_iterator(struct process_iterator *it, struct process_filter *filter)
{
	it->i = 0;
	/* Find out how much to allocate for it->pidlist */
	if ((it->count = proc_listpids(PROC_ALL_PIDS, 0, NULL, 0)) <= 0)
	{
		fprintf(stderr, "proc_listpids: %s\n", strerror(errno));
		return -1;
	}
	/* Allocate and populate it->pidlist */
	if ((it->pidlist = malloc((it->count) * sizeof(pid_t))) == NULL)
	{
		fprintf(stderr, "malloc: %s\n", strerror(errno));
		exit(-1);
	}
	if ((it->count = proc_listpids(PROC_ALL_PIDS, 0, it->pidlist, it->count)) <= 0)
	{
		fprintf(stderr, "proc_listpids: %s\n", strerror(errno));
		return -1;
	}
	it->count = unique_nonzero_pids(it->pidlist, it->count, it->pidlist);
	it->filter = filter;
	return 0;
}

static void pti2proc(struct proc_taskallinfo *ti, struct process *process)
{
	process->pid = ti->pbsd.pbi_pid;
	process->ppid = ti->pbsd.pbi_ppid;
	process->cputime = ti->ptinfo.pti_total_user / 1e6 + ti->ptinfo.pti_total_system / 1e6;
	if (ti->pbsd.pbi_name[0] != '\0')
	{
		process->max_cmd_len = MIN(sizeof(process->command), sizeof(ti->pbsd.pbi_name)) - 1;
		memcpy(process->command, ti->pbsd.pbi_name, process->max_cmd_len + 1);
	}
	else
	{
		process->max_cmd_len = MIN(sizeof(process->command), sizeof(ti->pbsd.pbi_comm)) - 1;
		memcpy(process->command, ti->pbsd.pbi_comm, process->max_cmd_len + 1);
	}
}

static int get_process_pti(pid_t pid, struct proc_taskallinfo *ti)
{
	int bytes;
	bytes = proc_pidinfo(pid, PROC_PIDTASKALLINFO, 0, ti, sizeof(*ti));
	if (bytes <= 0)
	{
		if (!(errno & (EPERM | ESRCH)))
		{
			fprintf(stderr, "proc_pidinfo: %s\n", strerror(errno));
		}
		return -1;
	}
	else if (bytes < (int)sizeof(ti))
	{
		fprintf(stderr, "proc_pidinfo: too few bytes; expected %lu, got %d\n", (unsigned long)sizeof(ti), bytes);
		return -1;
	}
	return 0;
}

pid_t getppid_of(pid_t pid)
{
	struct proc_taskallinfo ti;
	if (get_process_pti(pid, &ti) == 0)
	{
		return ti.pbsd.pbi_ppid;
	}
	return (pid_t)(-1);
}

int is_child_of(pid_t child_pid, pid_t parent_pid)
{
	if (child_pid <= 0 || parent_pid <= 0 || child_pid == parent_pid)
		return 0;
	while (child_pid > 1 && child_pid != parent_pid)
	{
		child_pid = getppid_of(child_pid);
	}
	return child_pid == parent_pid;
}

int get_next_process(struct process_iterator *it, struct process *p)
{
	if (it->i == it->count)
		return -1;
	if (it->filter->pid != 0 && !it->filter->include_children)
	{
		struct proc_taskallinfo ti;
		if (get_process_pti(it->filter->pid, &ti) != 0)
		{
			it->i = it->count = 0;
			return -1;
		}
		it->i = it->count = 1;
		pti2proc(&ti, p);
		return 0;
	}
	while (it->i < it->count)
	{
		struct proc_taskallinfo ti;
		if (get_process_pti(it->pidlist[it->i], &ti) != 0)
		{
			it->i++;
			continue;
		}
		if (ti.pbsd.pbi_flags & PROC_FLAG_SYSTEM)
		{
			it->i++;
			continue;
		}
		if (it->filter->pid != 0 && it->filter->include_children)
		{
			it->i++;
			pti2proc(&ti, p);
			if (p->pid != it->pidlist[it->i - 1]) /* I don't know why this can happen */
				continue;
			if (p->pid != it->filter->pid && !is_child_of(p->pid, it->filter->pid))
				continue;
			return 0;
		}
		else if (it->filter->pid == 0)
		{
			it->i++;
			pti2proc(&ti, p);
			return 0;
		}
	}
	return -1;
}

int close_process_iterator(struct process_iterator *it)
{
	free(it->pidlist);
	it->pidlist = NULL;
	it->filter = NULL;
	it->count = 0;
	it->i = 0;
	return 0;
}

#endif
#endif
