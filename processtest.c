#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include "process.h"

int main()
{
	struct process p;
	process_init(&p, getpid());
	struct cpu_usage u;
	process_monitor(&p, 0, &u);
	return 0;
}

