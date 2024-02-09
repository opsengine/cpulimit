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
 */

#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

#include "process_iterator.h"
#include "process_group.h"
#include "list.h"

#if defined(__linux__)
#if defined(CLOCK_TAI)
#define get_time(ts) clock_gettime(CLOCK_TAI, (ts))
#elif defined(CLOCK_MONOTONIC)
#define get_time(ts) clock_gettime(CLOCK_MONOTONIC, (ts))
#endif
#endif
#ifndef get_time
static int __get_time(struct timespec *ts)
{
	struct timeval tv;
	if (gettimeofday(&tv, NULL))
	{
		return -1;
	}
	ts->tv_sec = tv.tv_sec;
	ts->tv_nsec = tv.tv_usec * 1000L;
	return 0;
}
#define get_time(ts) __get_time(ts)
#endif

static char *__basename(char *path)
{
	char *p = strrchr(path, '/');
	return p != NULL ? p + 1 : path;
}
#define basename(path) __basename(path)

/* look for a process by pid
search_pid   : pid of the wanted process
return:  pid of the found process, if successful
		 negative pid, if the process does not exist or if the signal fails */
pid_t find_process_by_pid(pid_t pid)
{
	return (kill(pid, 0) == 0) ? pid : -pid;
}

/* look for a process with a given name
process: the name of the wanted process. it can be an absolute path name to the executable file
		or just the file name
return:  pid of the found process, if it is found
		0, if it's not found
		negative pid, if it is found but it's not possible to control it */
pid_t find_process_by_name(char *process_name)
{
	/* pid of the target process */
	pid_t pid = -1;

	/* process iterator */
	struct process_iterator it;
	struct process proc;
	struct process_filter filter;
	static char process_basename[PATH_MAX + 1];
	static char command_basename[PATH_MAX + 1];
	strncpy(process_basename, basename(process_name),
			sizeof(process_basename) - 1);
	process_basename[sizeof(process_basename) - 1] = '\0';
	filter.pid = 0;
	filter.include_children = 0;
	init_process_iterator(&it, &filter);
	while (get_next_process(&it, &proc) != -1)
	{
		int cmp_len;
		strncpy(command_basename, basename(proc.command),
				sizeof(command_basename) - 1);
		command_basename[sizeof(command_basename) - 1] = '\0';
		cmp_len = proc.max_cmd_len -
				  (strlen(proc.command) - strlen(command_basename));
		/* process found */
		if (cmp_len > 0 && command_basename[0] != '\0' &&
			strncmp(command_basename, process_basename, cmp_len) == 0)
		{
			if (pid < 0)
			{
				pid = proc.pid;
			}
			else if (is_child_of(pid, proc.pid))
			{
				pid = proc.pid;
			}
			else if (is_child_of(proc.pid, pid))
			{
			}
			else
			{
				pid = MIN(proc.pid, pid);
			}
		}
	}
	if (close_process_iterator(&it) != 0)
		exit(1);
	if (pid > 0)
	{
		/* the process was found */
		return find_process_by_pid(pid);
	}
	else
	{
		/* process not found */
		return 0;
	}
}

int init_process_group(struct process_group *pgroup, pid_t target_pid, int include_children)
{
	/* hashtable initialization */
	memset(&pgroup->proctable, 0, sizeof(pgroup->proctable));
	pgroup->target_pid = target_pid;
	pgroup->include_children = include_children;
	pgroup->proclist = malloc(sizeof(struct list));
	if (pgroup->proclist == NULL)
	{
		exit(-1);
	}
	init_list(pgroup->proclist, sizeof(pid_t));
	if (get_time(&pgroup->last_update))
	{
		exit(-1);
	}
	update_process_group(pgroup);
	return 0;
}

int close_process_group(struct process_group *pgroup)
{
	int i;
	int size = sizeof(pgroup->proctable) / sizeof(struct process *);
	for (i = 0; i < size; i++)
	{
		if (pgroup->proctable[i] != NULL)
		{
			/* free() history for each process */
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

/* returns t1-t2 in millisecond */
/* static inline double timediff_in_ms(const struct timespec *t1, const struct timespec *t2) */
#define timediff_in_ms(t1, t2) \
	(((t1)->tv_sec - (t2)->tv_sec) * 1e3 + ((t1)->tv_nsec - (t2)->tv_nsec) / 1e6)

/* parameter in range 0-1 */
#define ALFA 0.08
#define MIN_DT 20

void update_process_group(struct process_group *pgroup)
{
	struct process_iterator it;
	struct process tmp_process;
	struct process_filter filter;
	struct timespec now;
	double dt;
	if (get_time(&now))
	{
		exit(1);
	}
	/* time elapsed from previous sample (in ms) */
	dt = timediff_in_ms(&now, &pgroup->last_update);
	filter.pid = pgroup->target_pid;
	filter.include_children = pgroup->include_children;
	init_process_iterator(&it, &filter);
	clear_list(pgroup->proclist);
	init_list(pgroup->proclist, sizeof(pid_t));

	while (get_next_process(&it, &tmp_process) != -1)
	{
		int hashkey = pid_hashfn(tmp_process.pid);
		if (pgroup->proctable[hashkey] == NULL)
		{
			/* empty bucket */
			struct process *new_process = malloc(sizeof(struct process));
			if (new_process == NULL)
			{
				exit(-1);
			}
			pgroup->proctable[hashkey] = malloc(sizeof(struct list));
			if (pgroup->proctable[hashkey] == NULL)
			{
				exit(-1);
			}
			tmp_process.cpu_usage = -1;
			memcpy(new_process, &tmp_process, sizeof(struct process));
			init_list(pgroup->proctable[hashkey], sizeof(pid_t));
			add_elem(pgroup->proctable[hashkey], new_process);
			add_elem(pgroup->proclist, new_process);
		}
		else
		{
			/* existing bucket */
			struct process *p = (struct process *)locate_elem(pgroup->proctable[hashkey], &tmp_process);
			if (p == NULL)
			{
				/* process is new. add it */
				struct process *new_process = malloc(sizeof(struct process));
				if (new_process == NULL)
				{
					exit(-1);
				}
				tmp_process.cpu_usage = -1;
				memcpy(new_process, &tmp_process, sizeof(struct process));
				add_elem(pgroup->proctable[hashkey], new_process);
				add_elem(pgroup->proclist, new_process);
			}
			else
			{
				double sample;
				add_elem(pgroup->proclist, p);
				if (dt < MIN_DT)
					continue;
				/* process exists. update CPU usage */
				sample = (tmp_process.cputime - p->cputime) / dt;
				if (p->cpu_usage == -1)
				{
					/* initialization */
					p->cpu_usage = sample;
				}
				else
				{
					/* usage adjustment */
					p->cpu_usage = (1.0 - ALFA) * p->cpu_usage + ALFA * sample;
				}
				p->cputime = tmp_process.cputime;
			}
		}
	}
	close_process_iterator(&it);
	if (dt < MIN_DT)
		return;
	pgroup->last_update = now;
}

int remove_process(struct process_group *pgroup, pid_t pid)
{
	int hashkey = pid_hashfn(pid);
	struct list_node *node;
	if (pgroup->proctable[hashkey] == NULL)
		return 1; /* nothing to delete */
	node = (struct list_node *)locate_node(pgroup->proctable[hashkey], &pid);
	if (node == NULL)
		return 2;
	delete_node(pgroup->proctable[hashkey], node);
	return 0;
}
