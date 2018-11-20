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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>
#include <signal.h>
#include <string.h>

#ifdef __APPLE__ || __FREEBSD__
#include <libgen.h>
#endif

#include <process_iterator.h>
#include <process_group.h>

volatile sig_atomic_t child;

void kill_child(int sig)
{
	kill(child, SIGINT);
}

void test_single_process()
{
	struct process_iterator it;
	struct process process;
	struct process_filter filter;
	int count;
	//don't iterate children
	filter.pid = getpid();
	filter.include_children = 0;
	count = 0;
//	time_t now = time(NULL);
	init_process_iterator(&it, &filter);
	while (get_next_process(&it, &process) == 0)
	{
		assert(process.pid == getpid());
		assert(process.ppid == getppid());
		assert(process.cputime < 100);
//		assert(process.starttime == now || process.starttime == now - 1);
		count++;
	}
	assert(count == 1);
	close_process_iterator(&it);
	//iterate children
	filter.pid = getpid();
	filter.include_children = 0;
	count = 0;
//	now = time(NULL);
	init_process_iterator(&it, &filter);
	while (get_next_process(&it, &process) == 0)
	{
		assert(process.pid == getpid());
		assert(process.ppid == getppid());
		assert(process.cputime < 100);
//		assert(process.starttime == now || process.starttime == now - 1);
		count++;
	}
	assert(count == 1);
	close_process_iterator(&it);
}

void test_multiple_process()
{
	struct process_iterator it;
	struct process process;
	struct process_filter filter;
	pid_t child = fork();
	if (child == 0)
	{
		//child is supposed to be killed by the parent :/
		sleep(1);
		exit(1);
	}
	filter.pid = getpid();
	filter.include_children = 1;
	init_process_iterator(&it, &filter);
	int count = 0;
//	time_t now = time(NULL);
	while (get_next_process(&it, &process) == 0)
	{
		if (process.pid == getpid()) assert(process.ppid == getppid());
		else if (process.pid == child) assert(process.ppid == getpid());
		else assert(0);
		assert(process.cputime < 100);
//		assert(process.starttime == now || process.starttime == now - 1);
		count++;
	}
	assert(count == 2);
	close_process_iterator(&it);
	kill(child, SIGINT);
}

void test_all_processes()
{
	struct process_iterator it;
	struct process process;
	struct process_filter filter;
	filter.pid = 0;
	filter.include_children = 0;
	init_process_iterator(&it, &filter);
	int count = 0;
//	time_t now = time(NULL);
	while (get_next_process(&it, &process) == 0)
	{
		if (process.pid == getpid())
		{
			assert(process.ppid == getppid());
			assert(process.cputime < 100);
//			assert(process.starttime == now || process.starttime == now - 1);
		}
		count++;
	}
	assert(count >= 10);
	close_process_iterator(&it);
}

void test_process_group_all()
{
	struct process_group pgroup;
	assert(init_process_group(&pgroup, 0, 0) == 0);
	update_process_group(&pgroup);
	struct list_node *node = NULL;
	int count = 0;
	for (node=pgroup.proclist->first; node!= NULL; node=node->next) {
		count++;
	}
	assert(count > 10);
	update_process_group(&pgroup);
	assert(close_process_group(&pgroup) == 0);
}

void test_process_group_single(int include_children)
{
	struct process_group pgroup;
	child = fork();
	if (child == 0)
	{
		//child is supposed to be killed by the parent :/
		while(1);
		exit(1);
	}
	signal(SIGABRT, &kill_child);
    signal(SIGTERM, &kill_child);
	assert(init_process_group(&pgroup, child, include_children) == 0);
	int i;
	double tot_usage = 0;
	for (i=0; i<100; i++)
	{
		update_process_group(&pgroup);
		struct list_node *node = NULL;
		int count = 0;
		for (node=pgroup.proclist->first; node!= NULL; node=node->next) {
			struct process *p = (struct process*)(node->data);
			assert(p->pid == child);
			assert(p->ppid == getpid());
			assert(p->cpu_usage <= 1.2);
			tot_usage += p->cpu_usage;
			count++;
		}
		assert(count == 1);
		struct timespec interval;
		interval.tv_sec = 0;
		interval.tv_nsec = 50000000;
		nanosleep(&interval, NULL);
	}
	assert(tot_usage / i < 1.1 && tot_usage / i > 0.8);
	assert(close_process_group(&pgroup) == 0);
	kill(child, SIGINT);
}

void test_process_name(const char * command)
{
	struct process_iterator it;
	struct process process;
	struct process_filter filter;
	filter.pid = getpid();
	filter.include_children = 0;
	init_process_iterator(&it, &filter);
	assert(get_next_process(&it, &process) == 0);
	assert(process.pid == getpid());
	assert(process.ppid == getppid());
	#ifdef __APPLE__
	// proc_pidinfo only gives us the first 15 chars
	// of the basename of the command on OSX.
	assert(strncmp(basename((char*)command), process.command, 15) == 0);
	#else
	assert(strncmp(command, process.command, strlen(process.command)) == 0);
	#endif
	assert(get_next_process(&it, &process) != 0);
	close_process_iterator(&it);
}

void test_process_group_wrong_pid()
{
	struct process_group pgroup;
	assert(init_process_group(&pgroup, -1, 0) == 0);
	assert(pgroup.proclist->count == 0);
	update_process_group(&pgroup);
	assert(pgroup.proclist->count == 0);
	assert(init_process_group(&pgroup, 9999999, 0) == 0);
	assert(pgroup.proclist->count == 0);
	update_process_group(&pgroup);
	assert(pgroup.proclist->count == 0);
	assert(close_process_group(&pgroup) == 0);
}

int main(int argc, char **argv)
{
//	printf("Pid %d\n", getpid());
	test_single_process();
	test_multiple_process();
	test_all_processes();
	test_process_group_all();
	test_process_group_single(0);
	test_process_group_single(1);
	test_process_group_wrong_pid();
	test_process_name(argv[0]);
	return 0;
}
