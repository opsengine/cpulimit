#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

void *loop()
{
	while(1);
}

int main(int argc, char **argv) {

	int i = 0;
	int num_threads = 1;
	if (argc == 2) num_threads = atoi(argv[1]);
	for (i=0; i<num_threads-1; i++)
	{
		pthread_t thread;
		int ret;
		if ((ret = pthread_create(&thread, NULL, loop, NULL)) != 0)
		{
			printf("pthread_create() failed. Error code %d\n", ret);
			exit(1);
		}
	}
	loop();
	return 0;
}

