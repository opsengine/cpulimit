#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>

#define N 8

//simple program to test cpulimit
int main()
{
	printf("Parent: PID %d\n", getpid());
	getchar();
	sleep(1);
	while(1);
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
			while(1);
			//child code
//			while(1) {
				//random generator initialization
				struct timeval t;
				gettimeofday(&t, NULL);
				srandom(t.tv_sec + t.tv_usec + getpid());
				int loop = random() % 1000000;
				printf("start\n");
				int i,j;
				for (i=0;i<1000;i++)
					for (j=0;j<loop;j++);
				printf("stop\n");
				int time = random() % 10000000;
	//			printf("Child %d wait %d\n", getpid(), time);
				usleep(time);
				printf("Child %d terminated\n", getpid());
//			}
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
