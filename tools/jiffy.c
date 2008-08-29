#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>

#ifdef __APPLE__
#include <Carbon/Carbon.h>
#endif

static int get_jiffies(pid_t pid) {
#ifdef __GNUC__
	char buffer[1024];
	char stat_file[32];
	sprintf(stat_file, "/proc/%d/stat", pid);
	FILE *f = fopen(stat_file, "r");
	if (f==NULL) return -1;
	fgets(buffer, sizeof(buffer),f);
	fclose(f);
	char *p = buffer;
	p = memchr(p+1,')', sizeof(buffer) - (p-buffer));
	int sp = 12;
	while (sp--)
		p = memchr(p+1,' ',sizeof(buffer) - (p-buffer));
	//user mode jiffies
	int utime = atoi(p+1);
	p = memchr(p+1,' ',sizeof(buffer) - (p-buffer));
	//kernel mode jiffies
	int ktime = atoi(p+1);
	return utime+ktime;
#elif defined __APPLE__
	ProcessSerialNumber psn;
	ProcessInfoRec info;
	if (GetProcessForPID(pid, &psn)) return -1;
	if (GetProcessInformation(&psn, &info)) return -1;
	return info.processActiveTime;
#endif
}

struct timeval timeval_diff(struct timeval *tv1, struct timeval *tv2)
{
	struct timeval diff;
	diff.tv_sec = tv2->tv_sec - tv1->tv_sec;
	diff.tv_usec = tv2->tv_usec - tv1->tv_usec;
	if (diff.tv_usec < 0) {
		diff.tv_sec--;
		diff.tv_usec += 1000000;
	}
	return diff;
}

int main() {
	struct timeval start, now, diff;
	int i,j;
	int pid = getpid();
	gettimeofday(&start, NULL);
	printf("time     j   HZ        jiffy time\n");
	while(1) {
		for(i=0;i<100000;i++)
			for(j=0;j<10000;j++);
		int j = get_jiffies(pid);
		if (j<0) exit(-1);
		gettimeofday(&now, NULL);
		diff = timeval_diff(&start, &now);
		double d = diff.tv_sec + diff.tv_usec * 1e-6;
		printf("%lf %d %lf %lf ms\n", d, j, j/d, 1000*d/j);
	}
	exit(0);
}
