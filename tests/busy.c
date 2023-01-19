#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>

#define MAX_PRIORITY -20

static void increase_priority(void)
{
	/* find the best available nice value */
	int priority;
	setpriority(PRIO_PROCESS, 0, MAX_PRIORITY);
	priority = getpriority(PRIO_PROCESS, 0);
	while (priority > MAX_PRIORITY && setpriority(PRIO_PROCESS, 0, priority - 1) == 0)
	{
		priority--;
	}
}

/* Get the number of CPUs */
static int get_ncpu(void)
{
	int ncpu;
#if defined(_SC_NPROCESSORS_ONLN)
	ncpu = sysconf(_SC_NPROCESSORS_ONLN);
#elif defined(__APPLE__)
	int mib[2] = {CTL_HW, HW_NCPU};
	size_t len = sizeof(ncpu);
	sysctl(mib, 2, &ncpu, &len, NULL, 0);
#elif defined(_GNU_SOURCE)
	ncpu = get_nprocs();
#else
	ncpu = -1;
#endif
	return ncpu;
}

#ifdef __GNUC__
static void *loop(__attribute__((__unused__)) void *param)
#else
static void *loop(void *param)
#endif
{
	while (1)
		;
	return NULL;
}

int main(int argc, char *argv[])
{

	int i = 0;
	int num_threads = get_ncpu();
	increase_priority();
	if (argc == 2)
		num_threads = atoi(argv[1]);
	for (i = 0; i < num_threads - 1; i++)
	{
		pthread_t thread;
		int ret;
		if ((ret = pthread_create(&thread, NULL, loop, NULL)) != 0)
		{
			printf("pthread_create() failed. Error code %d\n", ret);
			exit(1);
		}
	}
	loop(NULL);
	return 0;
}
