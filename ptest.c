#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "procutils.h"

#define N 10

//simple program to test cpulimit
int main()
{
	printf("Parent: PID %d\n", getpid());
	getchar();
	int i;
	int children[N];
	for (i=0;i<N;i++) {
		int pid = fork();
		if (pid>0) {
			//parent code
			children[i] = pid;
			printf("Child %d created\n", pid);
		}
		else if (pid==0) {
			//child code
			struct timeval t;
			gettimeofday(&t, NULL);
			srandom(t.tv_usec + getpid());
			int time = random() % 2000000;
//			printf("Child %d wait %d\n", getpid(), time);
			usleep(time);
			printf("Child %d terminated\n", getpid());
			exit(0);
		}
		else {
			fprintf(stderr, "fork() failed!\n");
		}
	}
	for (i=0;i<N;i++) {
		int status;
		waitpid(children[i], &status, 0);
	}
	sleep(1);
	return 0;
}
