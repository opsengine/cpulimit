#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/procfs.h>

#include "process_iterator.h"

//TODO read this to port to other systems: http://www.steve.org.uk/Reference/Unix/faq_8.html#SEC85

#ifdef __linux__

int init_process_iterator(struct process_iterator *it) {
	//open a directory stream to /proc directory
	if ((it->dip = opendir("/proc")) == NULL) {
		perror("opendir");
		return -1;
	}
	return 0;
}

int read_next_process(struct process_iterator *it, struct process *p) {
	char statfile[20];
	char buffer[1024];
	//read in from /proc and seek for process dirs
	while ((it->dit = readdir(it->dip)) != NULL) {
		if(strtok(it->dit->d_name, "0123456789") != NULL)
			continue;
		p->pid = atoi(it->dit->d_name);
		//read stat file
		sprintf(statfile, "/proc/%d/stat", p->pid);
		FILE *fd = fopen(statfile, "r");
		if (fd==NULL) return -1;
		if (fgets(buffer, sizeof(buffer), fd)==NULL) {
			fclose(fd);
			return -1;
		}
		fclose(fd);
		char *token = strtok(buffer, " ");
		int i;
		for (i=0; i<3; i++) token = strtok(NULL, " ");
		p->ppid = atoi(token);
		for (i=0; i<10; i++)
			token = strtok(NULL, " ");
		p->last_jiffies = atoi(token);
		token = strtok(NULL, " ");
		p->last_jiffies += atoi(token);
		for (i=0; i<7; i++)
			token = strtok(NULL, " ");
		p->starttime = atoi(token);
		break;
		//read command line
		char exefile[20];
		sprintf(exefile,"/proc/%d/cmdline", p->pid);
		fd = fopen(statfile, "r");
		char buffer[1024];
		if (fgets(buffer, sizeof(buffer), fd)==NULL) {
			fclose(fd);
			return -1;
		}
		fclose(fd);
		sscanf(buffer, "%s", (char*)&p->command);
	}
	if (it->dit == NULL)
	{
		//end of processes
		closedir(it->dip);
		it->dip = NULL;
		return -1;
	}
	return 0;
}

int close_process_iterator(struct process_iterator *it) {
	if (it->dip != 0 && closedir(it->dip) == -1) {
		perror("closedir");
		return 1;
	}
	return 0;
}

#elif defined __APPLE__

#elif defined __FreeBSD__

#include <sys/sysctl.h>
#include <sys/user.h>

int init_process_iterator(struct process_iterator *it) {
	char errbuf[_POSIX2_LINE_MAX];
	/* Open the kvm interface, get a descriptor */
	if ((it->kd = kvm_open(NULL, NULL, NULL, 0, errbuf)) == NULL) {
		/* fprintf(stderr, "kvm_open: %s\n", errbuf); */
		fprintf(stderr, "kvm_open: %s", errbuf);
		return -1;
	}
	/* Get the list of processes. */
	if ((it->procs = kvm_getprocs(it->kd, KERN_PROC_PROC, 0, &it->count)) == NULL) {
		kvm_close(it->kd);
		/* fprintf(stderr, "kvm_getprocs: %s\n", kvm_geterr(kd)); */
		fprintf(stderr, "kvm_getprocs: %s", kvm_geterr(it->kd));
		return -1;
	}
	kvm_close(it->kd);
	it->i = 0;
	return 0;
}

int read_next_process(struct process_iterator *it, struct process *p) {
	if (it->i == it->count) return -1;
	p->pid = it->procs[it->i].ki_pid;
	p->ppid = it->procs[it->i].ki_ppid;
	p->last_jiffies = it->procs[it->i].ki_runtime / 1000;
	it->i++;
	return 0;
}

int close_process_iterator(struct process_iterator *it) {
	return 0;
}

#endif
