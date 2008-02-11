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

int look_for_process_by_pid(int pid)
{
	DIR *dip;
	struct dirent *dit;
	int ret=0;

	//open a directory stream to /proc directory
	if ((dip = opendir("/proc")) == NULL) {
		perror("opendir");
		return -10;
	}

	//read in from /proc and seek for process dirs
	while ((dit = readdir(dip)) != NULL) {
		//get pid
		if (pid==atoi(dit->d_name)) {
			//here is the process
			if (kill(pid,SIGCONT)==0) {
				//process is ok!
				ret = pid;
			}
			else {
				//process detected, but you aren't allowed to control it
				ret = -pid;
			}
			break;
		}
	}

	//close the dir stream and check for errors
	if (closedir(dip) == -1) {
		perror("closedir");
		return -11;
	}

	return ret;
}

//look for a process with a given name
//process: the name of the wanted process. it can be an absolute path name to the executable file
//         or just the file name
//return:  pid of the found process, if it is found
//         0, if it's not found
//         negative pid, if it is found but it's not possible to control it
int look_for_process_by_name(const char *process)
{
	//the name of /proc/pid/exe symbolic link pointing to the executable file
	char exelink[20];
	//the name of the executable file
	char exepath[PATH_MAX+1];
	//pid of the process, if it's found
	int pid = 0;
	//whether the variable process is the absolute path or not
	int is_absolute_path = process[0] == '/';
	//flag indicating if the a process with given name was found
	int found = 0;

	DIR *dip;
	struct dirent *dit;

	//open a directory stream to /proc directory
	if ((dip = opendir("/proc")) == NULL) {
		perror("opendir");
		return -10;
	}

	//read in from /proc and seek for process dirs
	while ((dit = readdir(dip)) != NULL) {
		//get pid
		pid = atoi(dit->d_name);
		if (pid>0) {
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
	}

	//close the dir stream and check for errors
	if (closedir(dip) == -1) {
		perror("closedir");
		return -11;
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
	//this can't ever happen
	return 0;

}

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

// searches a process in the tree by its pid
// returns the address of the node, or NULL if it's not found
static struct process_tree_node *locate_process(struct process_tree *ptree, int pid)
{
	struct process_tree_node *ret = NULL;
	int i;
	for (i=0; i<ptree->proc_size; i++)
		if (ptree->proc[i].pid>0 && ptree->proc[i].pid == pid)
			ret = &(ptree->proc[i]);
	return ret;
}

// builds the complete current process tree
// it should be called just once, then it's enough to call the faster update_process_tree()
// to keep the tree up to date
// if spid is set to a valid process pid, subprocesses monitoring will be activated
// returns 0 if successful
int build_process_tree(struct process_tree *ptree)
{
	return 0;
}

// updates the process tree
// you should call build_process_tree() once before using this function
// if spid is set to a valid process pid, even new_subsubprocesses is cleared and updated
// returns 0 if successful
int update_process_tree(struct process_tree *ptree)
{
	return 0;
}

// finds the subprocesses of a given process (children, children of children, etc..)
// the algorithm first finds the process and then, its subprocesses recursively
// you should call build_process_tree() once before using this function
// if pid is spid, consider using get_new_subprocesses() which is much faster and scalable O(1)
// returns the number of subprocesses found, or -1 if the pid it's not found in the tree
int get_subprocesses(struct process_tree *ptree, int pid, int **subprocesses, int *subprocesses_size)
{
	return 0;
}

// returns special process subprocesses detected by last build_process_tree() or update_process_tree() call
// you should call build_process_tree() once before using this function
int get_new_subprocesses(struct process_tree *ptree, int **subprocesses, int *subprocesses_size)
{
	*subprocesses_size = ptree->subprocesses_size;
	
	return 0;
}

// free the heap memory used by a process tree
void cleanup_process_tree(struct process_tree *ptree)
{
	int i;
	for (i=0; i<ptree->proc_size; i++) {
		//if it's a valid process then free the children
		if (ptree->proc[i].pid > 0 && ptree->proc[i].children_count > 0) {
			free(ptree->proc[i].children);
			ptree->proc[i].children = NULL;
		}
	}
	//free subprocesses of the special process
	if (ptree->new_subprocesses_size > 0) {
		free(ptree->new_subprocesses);
		ptree->new_subprocesses = NULL;
	}
	//free the nodes dynamic array
	free(ptree->proc);
	ptree->proc = NULL;
}

// searches the sub processes of a given process
// returns the number of subprocesses found
int get_sub_processes2(struct process_tree *ptree, int pid, int **sub)
{
	struct process_tree_node *p = locate_process(ptree, pid);
	int sub_block = 10;
	int sub_count = sub_block;
	*sub = malloc(sizeof(int) * sub_block);
}


//TODO: add a get_process_tree(), get_sub_processes(tree), update_process_tree()

static void visit_tree(struct process_tree_node *node, int **children, int *children_count)
{
	int i;
	for (i=0; i<node->children_count; i++) {
		(*children)[*children_count] = node->children[i]->pid;
		(*children_count)++;
		visit_tree(node->children[i], children, children_count);
	}
	return;
}

int get_sub_processes_old(int pid, int **children)
{
	//stuff for reading directories
	DIR *dip;
	struct dirent *dit;

	//open a directory stream to /proc directory
	if ((dip = opendir("/proc")) == NULL) {
		perror("opendir");
		return -10;
	}

	//initial size of process dynamic array
	int proc_block = 64;
	//dinamic array containing pids of the processes
	int *proc = malloc(sizeof(int) * proc_block);
	int proc_count = 0;
	int children_count = 0;

	//read in from /proc and seek for processes
	while ((dit = readdir(dip)) != NULL) {
		//parse pid
		int pid2 = atoi(dit->d_name);
		if (pid2 == 0) continue;	//not a process
		//check if the array must be enlarged
		if (proc_count>0 && (proc_count % proc_block == 0)) {
			proc = realloc(proc, sizeof(int) * (proc_count + proc_block));
		}		
		//add pid2 to proc
		proc[proc_count++] = pid2;
	}
	//close the dir stream and check for errors
	if (closedir(dip) == -1) {
		perror("closedir");
		return -11;
	}
	//shrink proc to the real size
	proc = realloc(proc, sizeof(int) * proc_count);
	//which members of proc are children in the process tree?
	//create the children array
	*children = malloc(sizeof(int) * proc_count);

	//process tree
	struct process_tree_node nodes[proc_count], *init_process=NULL, *main_process=NULL;

	printf("%d processes detected\n", proc_count);

	//counters
	int i, j;
	//initial size of dynamic children arrays
	int children_block = 8;
	//tree initialization
	for (i=0; i<proc_count; i++) {
		nodes[i].pid = proc[i];
		nodes[i].children_count = 0;
		nodes[i].children = malloc(sizeof(void*)*children_block);
	}
	//build the process tree
	for (i=0; i<proc_count; i++) {
		int ppid = getppid_of(proc[i]);
		//proc[i] is child of ppid
		//search ppid in the proc
		for (j=0; j<proc_count; j++) {
			if (proc[j] == ppid) break;
		}
		if (j==proc_count) {
			//ppid not found, so proc[i] must be the init process
			if (proc[i]==1) init_process = &(nodes[i]);
		}
		else {
			if (nodes[j].children_count>0 && (nodes[j].children_count % children_block == 0)) {
				nodes[j].children = realloc(nodes[j].children, sizeof(void*) * (nodes[j].children_count + children_block));
			}
			nodes[j].children[nodes[j].children_count] = &(nodes[i]);
			nodes[j].children_count++;
		}
		if (nodes[i].pid == pid) main_process = &(nodes[i]);
	}

/*	for (i=0; i<proc_count; i++) {
		if (nodes[i].children_count == 0) continue;
		printf("Process %d has %d children (", nodes[i].pid, nodes[i].children_count);
		for (j=0; j<nodes[i].children_count; j++) {
			printf("%d", nodes[i].children[j]->pid);
			if (j<nodes[i].children_count-1) printf(" ");
		}
		printf(")\n");
	}	
*/
	//get the children list
	visit_tree(main_process, children, &children_count);
	//shrink children to the real size
	*children = realloc(*children, sizeof(int) * children_count);

	//memory cleanup
	for (i=0; i<proc_count; i++) {
		free(nodes[i].children);
		nodes[i].children = NULL;
	}
	free(proc);
	proc = NULL;

	return children_count;
}

