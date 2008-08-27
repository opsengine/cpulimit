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

#include <sys/utsname.h>
#include "procutils.h"

/* PROCESS STATISTICS FUNCTIONS */

// returns pid of the parent process
static int getppid_of(int pid)
{
	char file[20];
	char buffer[1024];
	sprintf(file, "/proc/%d/stat", pid);
	FILE *fd = fopen(file, "r");
		if (fd==NULL) return -1;
	fgets(buffer, sizeof(buffer), fd);
	fclose(fd);
	char *p = buffer;
	p = memchr(p+1,')', sizeof(buffer) - (p-buffer));
	int sp = 2;
	while (sp--)
		p = memchr(p+1,' ',sizeof(buffer) - (p-buffer));
	//pid of the parent process
	int ppid = atoi(p+1);
	return ppid;
}

// returns the start time of a process (used with pid to identify a process)
static int get_starttime(int pid)
{
	char file[20];
	char buffer[1024];
	sprintf(file, "/proc/%d/stat", pid);
	FILE *fd = fopen(file, "r");
		if (fd==NULL) return -1;
	fgets(buffer, sizeof(buffer), fd);
	fclose(fd);
	char *p = buffer;
	p = memchr(p+1,')', sizeof(buffer) - (p-buffer));
	int sp = 20;
	while (sp--)
		p = memchr(p+1,' ',sizeof(buffer) - (p-buffer));
	//start time of the process
	int time = atoi(p+1);
	return time;
}

// detects whether a process is a kernel thread or not
static int is_kernel_thread(int pid)
{
	static char statfile[20];
	static char buffer[64];
	int ret;
	sprintf(statfile, "/proc/%d/statm", pid);
	FILE *fd = fopen(statfile, "r");
	if (fd==NULL) return -1;
	fgets(buffer, sizeof(buffer), fd);
	ret = strncmp(buffer,"0 0 0",3)==0;
	fclose(fd);
	return ret;
}

// returns 1 if pid is a user process, 0 otherwise
static int process_exists(int pid) {
	static char statfile[20];
	static char buffer[64];
	int ret;
	sprintf(statfile, "/proc/%d/statm", pid);
	FILE *fd = fopen(statfile, "r");
	if (fd==NULL) return 0;
	fgets(buffer, sizeof(buffer), fd);
	ret = strncmp(buffer,"0 0 0",3)!=0;
	fclose(fd);
	return ret;
}

/* PID HASH FUNCTIONS */

static int hash_process(struct process_family *f, struct process *p)
{
	int ret;
	struct list **l = &(f->hashtable[pid_hashfn(p->pid)]);
	if (*l == NULL) {
		//there is no process in this hashtable item
		//allocate the list
		*l = malloc(sizeof(struct list));
		init_list(*l, 4);
		add_elem(*l, p);
		ret = 0;
		f->count++;
	}
	else {
		//list already exists
		struct process *tmp = (struct process*)locate_elem(*l, p);
		if (tmp != NULL) {
			//TODO: should free() something? tmp? p?
			//update process info
			memcpy(tmp, p, sizeof(struct process));
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

static void unhash_process(struct process_family *f, int pid) {
	//remove process from hashtable
	struct list **l = &(f->hashtable[pid_hashfn(pid)]);
	if (*l == NULL)
		return; //nothing done
	struct list_node *node = locate_node(*l, &pid);
	if (node != NULL)
		destroy_node(*l, node);
	f->count--;
}

static struct process *seek_process(struct process_family *f, int pid)
{
	struct list **l = &(f->hashtable[pid_hashfn(pid)]);
	return (*l != NULL) ? (struct process*)locate_elem(*l, &pid) : NULL;
}

/*
static int is_member(struct process_family *f, int pid) {
	struct process *p = seek_process(f, pid);
	return (p!=NULL && p->member);
}

static int exists(struct process_family *f, int pid) {
	struct process *p = seek_process(f, pid);
	return p!=NULL;
}
*/

/* PROCESS ITERATOR STUFF */

// creates an object that browse all running processes
static int init_process_iterator(struct process_iterator *i) {
	//open a directory stream to /proc directory
	if ((i->dip = opendir("/proc")) == NULL) {
		perror("opendir");
		return -1;
	}
	return 0;
}

// reads the next user process from /process
// automatic closing if the end of the list is reached
static int read_next_process(struct process_iterator *i) {
	//read in from /proc and seek for process dirs
	int pid = 0;
	while ((i->dit = readdir(i->dip)) != NULL) {
		pid = atoi(i->dit->d_name);
		if (pid<=0 || is_kernel_thread(pid))
			continue;
		//return the first found process
		break;
	}
	if (pid == 0) {
		//no more processes, release resources
		closedir(i->dip);
	}
	return pid;
}

/* PUBLIC FUNCTIONS */

// searches for all the processes derived from father and stores them
// in the process family struct
int create_process_family(struct process_family *f, int father)
{
	//process list initialization (4 bytes key)
	init_list(&(f->members), 4);
	//hashtable initialization
	memset(&(f->hashtable), 0, sizeof(f->hashtable));
	f->count = 0;
	f->father = father;
	//process iterator
	struct process_iterator iter;
	init_process_iterator(&iter);
	int pid = 0;
	while ((pid = read_next_process(&iter))) {
		//check if process belongs to the family
		int ppid = pid;
		//TODO: optimize adding also these parents, and continue if process is already present
		while(ppid!=1 && ppid!=father) {
			ppid = getppid_of(ppid);
		}
		//allocate and insert the process
		struct process *p = malloc(sizeof(struct process));
		p->pid = pid;
		p->starttime = get_starttime(pid);
		if (ppid==1) {
			//the init process
			p->member = 0;
		}
		else if (pid != getpid()) {
			//add to members (but exclude the current cpulimit process!)
			p->member = 1;
			add_elem(&(f->members), p);
		}
		//init history
		p->history = malloc(sizeof(struct process_history));
		process_init(p->history, pid);
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
	while ((pid = read_next_process(&iter))) {
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
			printf("Fatal bug! Process %d is without parent\n", pid);
			exit(1);
		}
		//allocate and insert the process
		struct process *p = malloc(sizeof(struct process));
		p->pid = pid;
		p->starttime = get_starttime(pid);
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
		//init history
		p->history = malloc(sizeof(struct process_history));
		process_init(p->history, pid);
		//add to hashtable
		hash_process(f, p);
	}
	return ret;
}

// removes a process from the family by its pid
void remove_process_from_family(struct process_family *f, int pid)
{
	struct list_node *node = locate_node(&(f->members), &pid);
	if (node != NULL) {
		struct process *p = (struct process*)(node->data);
		free(p->history);
		p->history = NULL;
		destroy_node(&(f->members), node);
	}
	unhash_process(f, pid);
}

// free the heap memory used by a process family
void cleanup_process_family(struct process_family *f)
{
	int i;
	int size = sizeof(f->hashtable) / sizeof(struct process*);
	for (i=0; i<size; i++) {
		if (f->hashtable[i] != NULL) {
			//free() history for each process
			struct list_node *node = NULL;
			for (node=f->hashtable[i]->first; node!=NULL; node=node->next) {
				struct process *p = (struct process*)(node->data);
				free(p->history);
				p->history = NULL;
			}
			destroy_list(f->hashtable[i]);
			free(f->hashtable[i]);
			f->hashtable[i] = NULL;
		}
	}
	flush_list(&(f->members));
	f->count = 0;
	f->father = 0;
}

// look for a process by pid
// search_pid   : pid of the wanted process
// return:  pid of the found process, if it is found
//          0, if it's not found
//          negative pid, if it is found but it's not possible to control it
int look_for_process_by_pid(int pid)
{
	if (process_exists(pid))
		return (kill(pid,SIGCONT)==0) ? pid : -pid;
	return 0;
}

// look for a process with a given name
// process: the name of the wanted process. it can be an absolute path name to the executable file
//         or just the file name
// return:  pid of the found process, if it is found
//         0, if it's not found
//         negative pid, if it is found but it's not possible to control it
int look_for_process_by_name(const char *process)
{
	//the name of /proc/pid/exe symbolic link pointing to the executable file
	char exelink[20];
	//the name of the executable file
	char exepath[PATH_MAX+1];
	//whether the variable process is the absolute path or not
	int is_absolute_path = process[0] == '/';
	//flag indicating if the a process with given name was found
	int found = 0;

	//process iterator
	struct process_iterator iter;
	init_process_iterator(&iter);
	int pid = 0;
	while ((pid = read_next_process(&iter))) {
		//read the executable link
		sprintf(exelink,"/proc/%d/exe",pid);
		int size = readlink(exelink, exepath, sizeof(exepath));
		if (size>0) {
			found = 0;
			if (is_absolute_path && strncmp(exepath, process, size)==0 && size==strlen(process)) {
				//process found
				found = 1;
			}
			else {
				//process found
				if (strncmp(exepath + size - strlen(process), process, strlen(process))==0) {
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
	}
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
	return 0;
}
