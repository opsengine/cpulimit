#include <stdio.h>
#include <process_iterator.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>

int test_single_process()
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
	while (read_next_process(&it, &process) == 0)
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
	filter.include_children = 1;
	count = 0;
	now = time(NULL);
	init_process_iterator(&it, &filter);
	while (read_next_process(&it, &process) == 0)
	{
		assert(process.pid == getpid());
		assert(process.ppid == getppid());
		assert(process.cputime < 100);
		assert(process.starttime == now || process.starttime == now - 1);
		count++;
	}
	assert(count == 1);
	close_process_iterator(&it);
	return 0;
}

int test_multiple_process()
{
	struct process_iterator it;
	struct process process;
	struct process_filter filter;
	int child = fork();
	if (child == 0)
	{
		//child code
		sleep(1);
		return 0;
	}
	filter.pid = getpid();
	filter.include_children = 1;
	init_process_iterator(&it, &filter);
	int count = 0;
	time_t now = time(NULL);
	while (read_next_process(&it, &process) == 0)
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
	return 0;
}

int main()
{
	printf("Current PID %d\n", getpid());
	test_single_process();
	test_multiple_process();
	return 0;
}