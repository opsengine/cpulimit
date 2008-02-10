/**
 *
 * cpulimit - a cpu limiter for Linux
 *
 * Copyright (C) 2005-2008, by:  Angelo Marletta <marlonx80@hotmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 **********************************************************************
 *
 * This is a simple program to limit the cpu usage of a process
 * If you modify this code, send me a copy please
 *
 * Date:    26/12/2006
 * Version: 1.2
 * Get the latest version at: http://cpulimit.sourceforge.net/
 *
 * Todo:
 * - optimize compute_cpu_usage() function
 * - add threads and spawned processes control
 *
 * Changelog:
 *
 */


#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/resource.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>

//kernel time resolution (inverse of one jiffy interval) in Hertz
#define HZ sysconf(_SC_CLK_TCK)

//some useful macro
#define min(a,b) (a<b?a:b)
#define max(a,b) (a>b?a:b)

//pid of the controlled process
int pid=0;
//executable file name
char *program_name;
//verbose mode
int verbose=0;
//lazy mode
int lazy=0;
//priority values of the daemon (nice)
int high_priority_value = -10;
int low_priority_value = 10;
int default_priority_value;

//reverse byte search (I hate warnings)
void *memrchr(const void *s, int c, size_t n);

//how many processor do we have?
int get_processor_count() {
/*
	char stats[16 * 1024 + 1], *search;
	int fd, byteCount, cpuCount;

	int nr_cpu = 0;
	if (kstatfd > 0 && ioctl(kstatfd, IM_IOC_GETCPUNUM, &nr_cpu) >= 0) {
		cout << "# CPU: " << nr_cpu << endl;
		return nr_cpu;
	}

	fd = open("/proc/stat", O_RDONLY);
	if (fd < 0) {
		return 0;
	}
	byteCount = read(fd, stats + 1, sizeof(stats) - 1);
	close(fd);
	if ((byteCount < 0) || (byteCount == sizeof(stats) - 1)) {
		return 0;
	}
	stats[0] = '\n'; // Make the first line begin with a \n, like the others.
	stats[byteCount + 1] = '\0';
	search = stats;
	cpuCount = 0;
	while ((search = strstr(search, "\ncpu")) != NULL) {
		++cpuCount;
		++search; // Make sure we don't find the same CPU again!
	}

	// Decrease result by one, because on a single processor
	// machine, the code above counts "cpu" and "cpu0"!
	return cpuCount - 1;
	*/
	return 1;
}


//return ta-tb in microseconds (beware, no overflow checks!)
inline long timediff(const struct timespec *ta,const struct timespec *tb) {
	unsigned long us = (ta->tv_sec-tb->tv_sec)*1000000 + (ta->tv_nsec/1000 - tb->tv_nsec/1000);
	return us;
}

int waitforpid(int pid) {
	//switch to low priority
	if (setpriority(PRIO_PROCESS,getpid(),low_priority_value)!=0) {
		printf("Warning: cannot renice to %d\n", low_priority_value);
	}

	int i=0;

	while(1) {

		DIR *dip;
		struct dirent *dit;

		//open a directory stream to /proc directory
		if ((dip = opendir("/proc")) == NULL) {
			perror("opendir");
			return -1;
		}

		//read in from /proc and seek for process dirs
		while ((dit = readdir(dip)) != NULL) {
			//get pid
			if (pid==atoi(dit->d_name)) {
				//pid detected
				if (kill(pid,SIGSTOP)==0 &&  kill(pid,SIGCONT)==0) {
					//process is ok!
					goto done;
				}
				else {
					fprintf(stderr,"Error: Process %d detected, but you don't have permission to control it\n",pid);
				}
			}
		}

		//close the dir stream and check for errors
		if (closedir(dip) == -1) {
			perror("closedir");
			return -1;
		}

		//no suitable target found
		if (i++==0) {
			if (lazy) {
				fprintf(stderr,"No process found\n");
				exit(2);
			}
			else {
				printf("Warning: no target process found. Waiting for it...\n");
			}
		}

		//sleep for a while
		sleep(2);
	}

done:
	printf("Process %d detected\n",pid);
	//now set high priority, if possible
	if (setpriority(PRIO_PROCESS,getpid(),high_priority_value)!=0) {
		printf("Warning: cannot renice to %d\n", high_priority_value);
		if (setpriority(PRIO_PROCESS,getpid(),default_priority_value)!=0) {
			printf("Warning: cannot renice to %d\n", default_priority_value);
		}
	}
	return 0;

}

//this function periodically scans process list and looks for executable path names
//it should be executed in a low priority context, since precise timing does not matter
//if a process is found then its pid is returned
//process: the name of the wanted process, can be an absolute path name to the executable file
//         or simply its name
//return: pid of the found process
int getpidof(const char *process) {

	//set low priority
	if (setpriority(PRIO_PROCESS,getpid(),low_priority_value)!=0) {
		printf("Warning: cannot renice to %d\n", low_priority_value);
	}

	char exelink[20];
	char exepath[PATH_MAX+1];
	int pid=0;
	int i=0;

	while(1) {

		DIR *dip;
		struct dirent *dit;

		//open a directory stream to /proc directory
		if ((dip = opendir("/proc")) == NULL) {
			perror("opendir");
			return -1;
		}

		//read in from /proc and seek for process dirs
		while ((dit = readdir(dip)) != NULL) {
			//get pid
			pid=atoi(dit->d_name);
			if (pid>0) {
				sprintf(exelink,"/proc/%d/exe",pid);
				int size=readlink(exelink,exepath,sizeof(exepath));
				if (size>0) {
					int found=0;
					if (process[0]=='/' && strncmp(exepath,process,size)==0 && size==strlen(process)) {
						//process starts with / then it's an absolute path
						found=1;
					}
					else {
						//process is the name of the executable file
						if (strncmp(exepath+size-strlen(process),process,strlen(process))==0) {
							found=1;
						}
					}
					if (found==1) {
						if (kill(pid,SIGSTOP)==0 &&  kill(pid,SIGCONT)==0) {
							//process is ok!
							goto done;
						}
						else {
							fprintf(stderr,"Error: Process %d detected, but you don't have permission to control it\n",pid);
						}
					}
				}
			}
		}

		//close the dir stream and check for errors
		if (closedir(dip) == -1) {
			perror("closedir");
			return -1;
		}

		//no suitable target found
		if (i++==0) {
			if (lazy) {
				fprintf(stderr,"No process found\n");
				exit(2);
			}
			else {
				printf("Warning: no target process found. Waiting for it...\n");
			}
		}

		//sleep for a while
		sleep(2);
	}

done:
	printf("Process %d detected\n",pid);
	//now set high priority, if possible
	if (setpriority(PRIO_PROCESS,getpid(),high_priority_value)!=0) {
		printf("Warning: cannot renice to %d\n", high_priority_value);
		//we can't switch to high priority, set the default value
		if (setpriority(PRIO_PROCESS,getpid(),default_priority_value)!=0) {
			printf("Warning: cannot renice to %d\n", default_priority_value);
		}
	}
	return pid;

}

//SIGINT and SIGTERM signal handler
void quit(int sig) {
	//let the process continue if it's stopped
	kill(pid,SIGCONT);
	printf("Exiting...\n");
	exit(0);
}

//get used jiffies count from /proc filesystem
int getjiffies(int pid) {
	static char stat[20];
	static char buffer[1024];
	sprintf(stat,"/proc/%d/stat",pid);
	FILE *f=fopen(stat,"r");
	if (f==NULL) return -1;
	fgets(buffer,sizeof(buffer),f);
	fclose(f);
	char *p=buffer;
	p=memchr(p+1,')',sizeof(buffer)-(p-buffer));
	int sp=12;
	while (sp--)
		p=memchr(p+1,' ',sizeof(buffer)-(p-buffer));
	//user mode jiffies
	int utime=atoi(p+1);
	p=memchr(p+1,' ',sizeof(buffer)-(p-buffer));
	//kernel mode jiffies
	int ktime=atoi(p+1);
	return utime+ktime;
}

//process instant photo
struct process_screenshot {
	struct timespec when;	//timestamp
	int jiffies;	//jiffies count of the process
	int cputime;	//microseconds of work from previous screenshot to current
};

//extracted process statistics
struct cpu_usage {
	float pcpu;
	float workingrate;
};

//this function is an autonomous dynamic system
//it works with static variables (state variables of the system), that keep memory of recent past
//its aim is to estimate the cpu usage of the process
//to work properly it should be called in a fixed periodic way
//perhaps i will put it in a separate thread...
int compute_cpu_usage2(int pid,int last_working_quantum,struct cpu_usage *pusage) {
	#define MEM_ORDER 10
	//circular buffer containing last MEM_ORDER process screenshots
	static struct process_screenshot ps[MEM_ORDER];
	//the last screenshot recorded in the buffer
	static int front=-1;
	//the oldest screenshot recorded in the buffer
	static int tail=0;

	if (pusage==NULL) {
		//reinit static variables
		front=-1;
		tail=0;
		return -1;
	}

	//let's advance front index and save the screenshot
	front=(front+1)%MEM_ORDER;
	int j=getjiffies(pid);
	if (j>=0) ps[front].jiffies=j;
	else return -1;	//error: pid does not exist
	clock_gettime(CLOCK_REALTIME,&(ps[front].when));
	ps[front].cputime=last_working_quantum;

	//buffer actual size is: (front-tail+MEM_ORDER)%MEM_ORDER+1
	int size=(front-tail+MEM_ORDER)%MEM_ORDER+1;

	if (size==1) {
		//not enough samples taken (it's the first one!), return -1
		pusage->pcpu=-1;
		pusage->workingrate=1;
		return -1;
	}
	else {
		//now we can calculate cpu usage, interval dt and dtwork are expressed in microseconds
		long dt=timediff(&(ps[front].when),&(ps[tail].when));
		long dtwork=0;
		int i=(tail+1)%MEM_ORDER;
		int max=(front+1)%MEM_ORDER;
		do {
			dtwork+=ps[i].cputime;
			i=(i+1)%MEM_ORDER;
		} while (i!=max);
		int used=ps[front].jiffies-ps[tail].jiffies;
		float usage=(used*1000000.0/HZ)/dtwork;
		pusage->workingrate=1.0*dtwork/dt;
		pusage->pcpu=usage*pusage->workingrate;
		if (size==MEM_ORDER)
			tail=(tail+1)%MEM_ORDER;
		return 0;
	}
	#undef MEM_ORDER
}

int compute_cpu_usage(int pid,int last_working_quantum,struct cpu_usage *pusage) {

	static struct process_screenshot ps;
	int jiffies;

	if (pusage==NULL) {
		//reinit static variables
		return -1;
	}

	//get the screenshot of the process
	jiffies = getjiffies(pid);
	if (jiffies <0 )
		return -1; // pid does not exist
	ps.jiffies = jiffies;
	clock_gettime(CLOCK_REALTIME,&(ps.when));
	ps.cputime=last_working_quantum;

	//buffer actual size is: (front-tail+MEM_ORDER)%MEM_ORDER+1
	int size=(front-tail+MEM_ORDER)%MEM_ORDER+1;

	if (size==1) {
		//not enough samples taken (it's the first one!), return -1
		pusage->pcpu=-1;
		pusage->workingrate=1;
		return -1;
	}
	else {
		//now we can calculate cpu usage, interval dt and dtwork are expressed in microseconds
		long dt=timediff(&(ps[front].when),&(ps[tail].when));
		long dtwork=0;
		int i=(tail+1)%MEM_ORDER;
		int max=(front+1)%MEM_ORDER;
		do {
			dtwork+=ps[i].cputime;
			i=(i+1)%MEM_ORDER;
		} while (i!=max);
		int used=ps[front].jiffies-ps[tail].jiffies;
		float usage=(used*1000000.0/HZ)/dtwork;
		pusage->workingrate=1.0*dtwork/dt;
		pusage->pcpu=usage*pusage->workingrate;
		if (size==MEM_ORDER)
			tail=(tail+1)%MEM_ORDER;
		return 0;
	}
	#undef MEM_ORDER
}

void print_caption() {
	printf("\n%%CPU\twork quantum\tsleep quantum\tactive rate\n");
}

void print_usage(FILE *stream,int exit_code) {
	fprintf(stream, "Usage: %s TARGET [OPTIONS...]\n",program_name);
	fprintf(stream, "   TARGET must be exactly one of these:\n");
	fprintf(stream, "      -p, --pid=N        pid of the process\n");
	fprintf(stream, "      -e, --exe=FILE     name of the executable program file\n");
	fprintf(stream, "      -P, --path=PATH    absolute path name of the executable program file\n");
	fprintf(stream, "   OPTIONS\n");
	fprintf(stream, "      -l, --limit=N      percentage of cpu allowed from 0 to 100 (mandatory)\n");
	fprintf(stream, "      -v, --verbose      show control statistics\n");
	fprintf(stream, "      -z, --lazy         exit if there is no suitable target process, or if it dies\n");
	fprintf(stream, "      -h, --help         display this help and exit\n");
	exit(exit_code);
}

int main(int argc, char **argv) {

	printf("%d\n", HZ);
	exit(0);

	//get program name
	char *p=(char*)memrchr(argv[0],(unsigned int)'/',strlen(argv[0]));
	program_name = p==NULL?argv[0]:(p+1);
	//parse arguments
	int next_option;
	/* A string listing valid short options letters. */
	const char* short_options="p:e:P:l:vzh";
	/* An array describing valid long options. */
	const struct option long_options[] = {
		{ "pid", 0, NULL, 'p' },
		{ "exe", 1, NULL, 'e' },
		{ "path", 0, NULL, 'P' },
		{ "limit", 0, NULL, 'l' },
		{ "verbose", 0, NULL, 'v' },
		{ "lazy", 0, NULL, 'z' },
		{ "help", 0, NULL, 'h' },
		{ NULL, 0, NULL, 0 }
	};
	//argument variables
	const char *exe=NULL;
	const char *path=NULL;
	int perclimit=0;
	int pid_ok=0;
	int process_ok=0;
	int limit_ok=0;

	do {
		next_option = getopt_long (argc, argv, short_options,long_options, NULL);
		switch(next_option) {
			case 'p':
				pid=atoi(optarg);
				pid_ok=1;
				break;
			case 'e':
				exe=optarg;
				process_ok=1;
				break;
			case 'P':
				path=optarg;
				process_ok=1;
				break;
			case 'l':
				perclimit=atoi(optarg);
				limit_ok=1;
				break;
			case 'v':
				verbose=1;
				break;
			case 'z':
				lazy=1;
				break;
			case 'h':
				print_usage (stdout, 1);
				break;
			case '?':
				print_usage (stderr, 1);
				break;
			case -1:
				break;
			default:
				abort();
		}
	} while(next_option != -1);

	if (!process_ok && !pid_ok) {
		fprintf(stderr,"Error: You must specify a target process\n");
		print_usage (stderr, 1);
		exit(1);
	}
	if ((exe!=NULL && path!=NULL) || (pid_ok && (exe!=NULL || path!=NULL))) {
		fprintf(stderr,"Error: You must specify exactly one target process\n");
		print_usage (stderr, 1);
		exit(1);
	}
	if (!limit_ok) {
		fprintf(stderr,"Error: You must specify a cpu limit\n");
		print_usage (stderr, 1);
		exit(1);
	}
	float limit=perclimit/100.0;
	if (limit<0 || limit >1) {
		fprintf(stderr,"Error: limit must be in the range 0-100\n");
		print_usage (stderr, 1);
		exit(1);
	}
	//parameters are all ok!
	signal(SIGINT,quit);
	signal(SIGTERM,quit);

	default_priority_value = getpriority(PRIO_PROCESS, getpid());

	//time quantum in microseconds. it's splitted in a working period and a sleeping one
	int period=100000;
	struct timespec twork,tsleep;   //working and sleeping intervals
	memset(&twork,0,sizeof(struct timespec));
	memset(&tsleep,0,sizeof(struct timespec));

wait_for_process:

	//look for the target process..or wait for it
	if (exe!=NULL)
		pid=getpidof(exe);
	else if (path!=NULL)
		pid=getpidof(path);
	else {
		waitforpid(pid);
	}
	//process detected...let's go

	//init compute_cpu_usage internal stuff
	compute_cpu_usage(0,0,NULL);
	//main loop counter
	int i=0;

	struct timespec startwork,endwork;
	long workingtime=0;		//last working time in microseconds

	if (verbose) print_caption();

	float pcpu_avg=0;

	//here we should already have high priority, for time precision
	while(1) {

		//estimate how much the controlled process is using the cpu in its working interval
		struct cpu_usage cu;
		if (compute_cpu_usage(pid,workingtime,&cu)==-1) {
			fprintf(stderr,"Process %d dead!\n",pid);
			if (lazy) exit(2);
			//wait until our process appears
			goto wait_for_process;
		}

		//cpu actual usage of process (range 0-1)
		float pcpu=cu.pcpu;
		//rate at which we are keeping active the process (range 0-1)
		float workingrate=cu.workingrate;

		//adjust work and sleep time slices
		if (pcpu>0) {
			twork.tv_nsec=min(period*limit*1000/pcpu*workingrate,period*1000);
		}
		else if (pcpu==0) {
			twork.tv_nsec=period*1000;
		}
		else if (pcpu==-1) {
			//not yet a valid idea of cpu usage
			pcpu=limit;
			workingrate=limit;
			twork.tv_nsec=min(period*limit*1000,period*1000);
		}
		tsleep.tv_nsec=period*1000-twork.tv_nsec;

		//update average usage
		pcpu_avg=(pcpu_avg*i+pcpu)/(i+1);

		if (verbose && i%10==0 && i>0) {
			printf("%0.2f%%\t%6ld us\t%6ld us\t%0.2f%%\n",pcpu*100,twork.tv_nsec/1000,tsleep.tv_nsec/1000,workingrate*100);
		}

		if (limit<1 && limit>0) {
			//resume process
			if (kill(pid,SIGCONT)!=0) {
				fprintf(stderr,"Process %d dead!\n",pid);
				if (lazy) exit(2);
				//wait until our process appears
				goto wait_for_process;
			}
		}

		clock_gettime(CLOCK_REALTIME,&startwork);
		nanosleep(&twork,NULL);		//now process is working
		clock_gettime(CLOCK_REALTIME,&endwork);
		workingtime=timediff(&endwork,&startwork);

		if (limit<1) {
			//stop the process, it has worked enough
			if (kill(pid,SIGSTOP)!=0) {
				fprintf(stderr,"Process %d dead!\n",pid);
				if (lazy) exit(2);
				//wait until the process appears
				goto wait_for_process;
			}
			nanosleep(&tsleep,NULL);	//now process is sleeping
		}
		i++;
	}

}
