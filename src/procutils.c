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
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysctl.h>

#include "procutils.h"

#ifdef __APPLE__
#include <sys/sysctl.h>
#include <errno.h>
#endif

#ifdef __FreeBSD__
#include <kvm.h>
#endif

/* PROCESS STATISTICS FUNCTIONS */

//deprecated
// returns pid of the parent process
static pid_t getppid_of(pid_t pid)
{
#ifdef __linux__
	char file[20];
	char buffer[1024];
	sprintf(file, "/proc/%d/stat", pid);
	FILE *fd = fopen(file, "r");
		if (fd==NULL) return -1;
	if (fgets(buffer, sizeof(buffer), fd)==NULL) return -1;
	fclose(fd);
	char *p = buffer;
	p = memchr(p+1,')', sizeof(buffer) - (p-buffer));
	int sp = 2;
	while (sp--)
		p = memchr(p+1,' ',sizeof(buffer) - (p-buffer));
	//pid of the parent process
	pid_t ppid = atoi(p+1);
	return ppid;
#elif defined __APPLE__
	struct process p;
	get_proc_info(&p, pid);
	return p.ppid;
#elif defined __FreeBSD__
	
#endif
}

#ifdef __linux__
// detects whether a process is a kernel thread or not
static int is_kernel_thread(pid_t pid)
{
	static char statfile[20];
	static char buffer[64];
	int ret;
	sprintf(statfile, "/proc/%d/statm", pid);
	FILE *fd = fopen(statfile, "r");
	if (fd==NULL) return -1;
	if (fgets(buffer, sizeof(buffer), fd)==NULL) return -1;
	ret = strncmp(buffer,"0 0 0",3)==0;
	fclose(fd);
	return ret;
}
#endif

/* PID HASH FUNCTIONS */

static int hash_process(struct process_family *f, struct process *p)
{
	int ret;
	struct list **l = &(f->proctable[pid_hashfn(p->pid)]);
	if (*l == NULL) {
		//there is no process in this hashtable item
		//allocate the list
		*l = (struct list*)malloc(sizeof(struct list));
		init_list(*l, 4);
		add_elem(*l, p);
		ret = 0;
		f->count++;
	}
	else {
		//list already exists
		struct process *tmp = (struct process*)locate_elem(*l, p);
		if (tmp != NULL) {
			//update process info
			memcpy(tmp, p, sizeof(struct process));
			free(p);
			p = NULL;
			ret = 1;
		}
		else {
			//add new process
			add_elem(*l, p);
			ret = 0;
			f->count++;
		}
	}
	return ret;
}

static void unhash_process(struct process_family *f, pid_t pid) {
	//remove process from hashtable
	struct list **l = &(f->proctable[pid_hashfn(pid)]);
	if (*l == NULL)
		return; //nothing done
	struct list_node *node = locate_node(*l, &pid);
	if (node != NULL)
		destroy_node(*l, node);
	f->count--;
}

static struct process *seek_process(struct process_family *f, pid_t pid)
{
	struct list **l = &(f->proctable[pid_hashfn(pid)]);
	return (*l != NULL) ? (struct process*)locate_elem(*l, &pid) : NULL;
}

/* PROCESS ITERATOR STUFF */

// creates an object that browse all running processes
int init_process_iterator(struct process_iterator *i) {
#ifdef __linux__
	//open a directory stream to /proc directory
	if ((i->dip = opendir("/proc")) == NULL) {
		perror("opendir");
		return -1;
	}
#elif defined __APPLE__

	int err;
	struct kinfo_proc *result = NULL;
	size_t length;
	int mib[] = {CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0};

	/* We start by calling sysctl with result == NULL and length == 0.
	   That will succeed, and set length to the appropriate length.
	   We then allocate a buffer of that size and call sysctl again
	   with that buffer.
	*/
	length = 0;
	err = sysctl(mib, 4, NULL, &length, NULL, 0);
	if (err == -1) {
		err = errno;
	}
	if (err == 0) {
		result = malloc(length);
		err = sysctl(mib, 4, result, &length, NULL, 0);
		if (err == -1)
			err = errno;
		if (err == ENOMEM) {
			free(result); /* clean up */
			result = NULL;
		}
	}

	i->procList = result;
	i->count = err == 0 ? length / sizeof *result : 0;
	i->c = 0;
#elif defined __FreeBSD__
	kvm_t *kd;
	struct kinfo_proc *procs = NULL;
	char errbuf[_POSIX2_LINE_MAX];
	int count;
	/* Open the kvm interface, get a descriptor */
	if ((kd = kvm_open(NULL, NULL, NULL, 0, errbuf)) == NULL) {
		/* fprintf(stderr, "kvm_open: %s\n", errbuf); */
		fprintf(stderr, "kvm_open: %s", errbuf);
	}
	/* Get the list of processes. */
	if ((procs = kvm_getprocs(kd, KERN_PROC_ALL, 0, &count)) == NULL) {
		kvm_close(kd);
		/* fprintf(stderr, "kvm_getprocs: %s\n", kvm_geterr(kd)); */
		fprintf(stderr, "kvm_getprocs: %s", kvm_geterr(kd));
	}

#endif
	i->current = (struct process*)calloc(1, sizeof(struct process));
	return 0;
}

int close_process_iterator(struct process_iterator *i) {
#ifdef __linux__
	if (closedir(i->dip) == -1) {
		perror("closedir");
		return 1;
	}
#elif defined __APPLE__
	//TODO
#endif
	return 0;
}

// reads the next user process from /process
// automatic closing if the end of the list is reached
int read_next_process(struct process_iterator *i) {
#ifdef __linux__
	pid_t pid = 0;
//TODO read this to port to other systems: http://www.steve.org.uk/Reference/Unix/faq_8.html#SEC85
	//read in from /proc and seek for process dirs
	while ((i->dit = readdir(i->dip)) != NULL) {
		if(strtok(i->dit->d_name, "0123456789") != NULL)
			continue;
		pid = atoi(i->dit->d_name);
		if (is_kernel_thread(pid))
			continue;
		//return the first found process
		break;
	}
	if (pid == 0) {
		//no more processes
		closedir(i->dip);
		free(i->current);
		i->current = NULL;
		return -1;
	}
	//read the executable link
	char statfile[20];
	sprintf(statfile,"/proc/%d/cmdline",pid);
	FILE *fd = fopen(statfile, "r");
	if (fd == NULL) return -1;
	char buffer[1024];
	if (fgets(buffer, sizeof(buffer), fd)==NULL) return -2;
	fclose(fd);
	sscanf(buffer, "%s", (char*)&i->current->command);
	i->current->pid = pid;
	
#elif defined __APPLE__
	if (i->c >= i->count) {
		//no more processes
		free(i->procList);
		i->procList = NULL;
		free(i->current);
		i->current = NULL;
		return -1;
	}
	i->current->pid = i->procList[i->c].kp_proc.p_pid;
	strncpy(i->current->command, i->procList[i->c].kp_proc.p_comm, MAXCOMLEN);
printf("%d %d %s\n", i->c, i->current->pid, i->current->command);//i->procList[i->c].kp_proc.p_comm);
//printf("%d %d %s\n", i->c, i->current->pid, i->proc[i->c].kp_proc.p_comm);
	i->c++;
#endif
	return 0;
}

/* PUBLIC FUNCTIONS */

// search for all the processes derived from father and stores them
// in the process family struct
int create_process_family(struct process_family *f, pid_t father)
{
	//process list initialization (4 bytes key)
	init_list(&(f->members), 4);
	//hashtable initialization
	memset(&(f->proctable), 0, sizeof(f->proctable));
	f->count = 0;
	f->father = father;
	//process iterator
	struct process_iterator iter;
	init_process_iterator(&iter);
	int pid = 0;
	while (read_next_process(&iter)==0) {
		pid = iter.current->pid;
		//check if process belongs to the family
		int ppid = pid;
		//TODO: optimize adding also these parents, and continue if process is already present
		while(ppid!=1 && ppid!=father) {
			ppid = getppid_of(ppid);
		}
		//allocate process descriptor
		struct process *p = (struct process*)calloc(1, sizeof(struct process));
		//init process
		process_init(p, pid);
		if (ppid==1) {
			//the init process
			p->member = 0;
		}
		else if (pid != getpid()) {
			//add to members (but exclude the current cpulimit process!)
			p->member = 1;
			add_elem(&(f->members), p);
		}
		//add to hashtable
		hash_process(f, p);
	}
	return 0;
}

// checks if there are new processes born in the specified family
// if any they are added to the members list
// the number of new born processes is returned
int update_process_family(struct process_family *f)
{
	int ret = 0;
	//process iterator
	struct process_iterator iter;
	init_process_iterator(&iter);
	int pid = 0;
	while (read_next_process(&iter)==0) {
		pid = iter.current->pid;
		struct process *newp = seek_process(f, pid);
		if (newp != NULL) continue; //already known //TODO: what if newp is a new process with the same PID??
		//the process is new, check if it belongs to the family
		int ppid = getppid_of(pid);
		//search the youngest known ancestor of the process
		struct process *ancestor = NULL;
		while((ancestor=seek_process(f, ppid))==NULL) {
			ppid = getppid_of(ppid);
		}
		if (ancestor == NULL) {
			//this should never happen! if does, find and correct the bug
			fprintf(stderr, "Fatal bug! Process %d is without parent\n", pid);
			exit(1);
		}
		//allocate and insert the process
		struct process *p = (struct process*)calloc(1, sizeof(struct process));
		//init process
		process_init(p, pid);
		if (ancestor->member) {
			//add to members
			p->member = 1;
			add_elem(&(f->members), p);
			ret++;
		}
		else {
			//not a member
			p->member = 0;
		}
		//add to hashtable
		hash_process(f, p);
	}
	return ret;
}

// removes a process from the family by its pid
void remove_process_from_family(struct process_family *f, pid_t pid)
{
	struct list_node *node = locate_node(&(f->members), &pid);
	if (node != NULL) {
//		struct process *p = (struct process*)(node->data);
//		free(p->history);
//		p->history = NULL;
		delete_node(&(f->members), node);
	}
	unhash_process(f, pid);
}

// free the heap memory used by a process family
void cleanup_process_family(struct process_family *f)
{
	int i;
	int size = sizeof(f->proctable) / sizeof(struct process*);
	for (i=0; i<size; i++) {
		if (f->proctable[i] != NULL) {
			//free() history for each process
			struct list_node *node = NULL;
			for (node=f->proctable[i]->first; node!=NULL; node=node->next) {
//				struct process *p = (struct process*)(node->data);
//				free(p->history);
//				p->history = NULL;
			}
			destroy_list(f->proctable[i]);
			free(f->proctable[i]);
			f->proctable[i] = NULL;
		}
	}
	flush_list(&(f->members));
	f->count = 0;
	f->father = 0;
}

// look for a process by pid
// search_pid   : pid of the wanted process
// return:  pid of the found process, if successful
//          negative pid, if the process does not exist or if the signal fails
int look_for_process_by_pid(pid_t pid)
{
	return (kill(pid,0)==0) ? pid : -pid;
}

// look for a process with a given name
// process: the name of the wanted process. it can be an absolute path name to the executable file
//         or just the file name
// return:  pid of the found process, if it is found
//         0, if it's not found
//         negative pid, if it is found but it's not possible to control it
int look_for_process_by_name(const char *process_name)
{
	//whether the variable process_name is the absolute path or not
	int is_absolute_path = process_name[0] == '/';
	//flag indicating if the a process with given name was found
	int found = 0;

	//process iterator
	struct process_iterator iter;
	init_process_iterator(&iter);
	pid_t pid = 0;

	while (read_next_process(&iter)==0) {
		pid = iter.current->pid;
	
		int size = strlen(iter.current->command);

		found = 0;
		if (is_absolute_path && strncmp(iter.current->command, process_name, size)==0 && size==strlen(process_name)) {
			//process found
			found = 1;
		}
		else {
			//process found
			if (strncmp(iter.current->command + size - strlen(process_name), process_name, strlen(process_name))==0) {
				found = 1;
			}
		}
		if (found==1) {
			if (kill(pid,SIGCONT)==0) {
				//process is ok!
				break;
			}
			else {
				//we don't have permission to send signal to that process
				//so, don't exit from the loop and look for another one with the same name
				found = -1;
			}
		}
	}
	if (close_process_iterator(&iter) != 1) exit(1);
	if (found == 1) {
		//ok, the process was found
		return pid;
	}
	else if (found == 0) {
		//no process found
		return 0;
	}
	else if (found == -1) {
		//the process was found, but we haven't permission to control it
		return -pid;
	}
	//this MUST NOT happen
	exit(2);
}

