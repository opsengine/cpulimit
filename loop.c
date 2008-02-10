#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

void *loop() {
	printf("thread PID: %d\n", getpid());
	while(1);
}

int main()
{
	printf("PID: %d\n", getpid());
	pthread_t th;
	pthread_create(&th, NULL, loop, NULL);
	while(1);
	return 0;
}
