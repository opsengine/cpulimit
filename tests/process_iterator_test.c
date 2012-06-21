#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>
#include <signal.h>

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
	assert(tot_usage / i < 1.1 && tot_usage / i > 0.9);
	assert(close_process_group(&pgroup) == 0);
	kill(child, SIGINT);
}

int main()
{
//	printf("Pid %d\n", getpid());
	test_single_process();
	test_multiple_process();
	test_all_processes();
	test_process_group_all();
	test_process_group_single(0);
	test_process_group_single(1);
	return 0;
}
