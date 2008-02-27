#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "procutils.h"

//simple program to test cpulimit

int main()
{
	printf("PID %d\n", getpid());
	int i;
	for (i=0;i<10;i++) {
		if (fork()>0){
			printf("PID %d\n", getpid());
			while(1);
		}
	}
	return 0;
}
