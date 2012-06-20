#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>

#include <process_iterator.h>
#include <process_group.h>

void test_single_process()
{
	struct process_iterator it;
	struct process process;
	struct process_filter filter;
	int count;
	time_t now;
	//don't iterate children
	filter.pid = getpid();
	filter.include_children = 0;
	count = 0;
	now = time(NULL);
	init_process_iterator(&it, &filter);
	while (get_next_process(&it, &process) == 0)
	{
		assert(process.pid == getpid());
		assert(process.ppid == getppid());
		assert(process.cputime < 100);
		assert(process.starttime == now || process.starttime == now - 1);
		count++;
	}
	assert(count == 1);
	close_process_iterator(&it);
	//iterate children
	filter.pid = getpid();
	filter.include_children = 0;
	count = 0;
	now = time(NULL);
	init_process_iterator(&it, &filter);
	while (get_next_process(&it, &process) == 0)
	{
		assert(process.pid == getpid());
		assert(process.ppid == getppid());
		assert(process.cputime < 100);
		assert(process.starttime == now || process.starttime == now - 1);
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
	int child = fork();
	if (child == 0)
	{
		//child code
		sleep(1);
		exit(0);
	}
	filter.pid = getpid();
	filter.include_children = 1;
	init_process_iterator(&it, &filter);
	int count = 0;
	time_t now = time(NULL);
	while (get_next_process(&it, &process) == 0)
	{
		if (process.pid == getpid()) assert(process.ppid == getppid());
		else if (process.pid == child) assert(process.ppid == getpid());
		else assert(0);
		assert(process.cputime < 100);
		assert(process.starttime == now || process.starttime == now - 1);
		count++;
	}
	assert(count == 2);
	close_process_iterator(&it);
}

void test_all_processes()
{
	struct process_iterator it;
	struct process process;
	struct process_filter filter;
	filter.pid = 0;
	init_process_iterator(&it, &filter);
	int count = 0;
	time_t now = time(NULL);
	while (get_next_process(&it, &process) == 0)
	{
		if (process.pid == getpid())
		{
			assert(process.ppid == getppid());
			assert(process.cputime < 100);
			assert(process.starttime == now || process.starttime == now - 1);
		}
		count++;
	}
	assert(count >= 10);
	close_process_iterator(&it);
}

void test_process_list()
{
	struct process_group pgroup;
	pid_t current_pid = getpid();
	assert(init_process_group(&pgroup, current_pid, 0) == 0);
	update_process_group(&pgroup);
	assert(close_process_group(&pgroup) == 0);
}

int main()
{
	printf("Pid %d\n", getpid());
	test_single_process();
	test_multiple_process();
	test_all_processes();
	test_process_list();
	return 0;
}
