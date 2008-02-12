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

#ifndef __PROCUTILS_H

#define __PROCUTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include "list.h"

// a node contained in the process_tree
struct process_tree_node {
	//the pid of the process
	//if pid<=0 it means that the object doesn't contain any value
	int pid;
	//the time when the process started since system boot (measured in jiffies)
	int starttime;
	//the children nodes (list of struct process_tree_node)
	struct list *children;
};

// the process tree	
struct process_tree {
	//init, the father of each userspace process
	struct process_tree_node *user_root;
	//kthreadd, the father of each kernel thread
	struct process_tree_node *kernel_root;
	//dynamic list containing all the nodes (list of struct process_tree_node)
	struct list processes;
	//this is a special monitored process
	//if spid>0 when calling build_process_tree() or update_process_tree()
	//new detected subprocesses of this process will be added in new_subprocesses array
	int spid;
	//new subprocesses of the special process (list of struct process_tree_node)
	struct list new_subprocesses;
};

// builds the complete current process tree
// it should be called just once, then it's enough to call the faster update_process_tree()
// to keep the tree up to date
// if spid is set to a valid process pid, subprocesses monitoring will be activated
// returns 0 if successful
int build_process_tree(struct process_tree *ptree);

// updates the process tree
// you should call build_process_tree() once before using this function
// if spid is set to a valid process pid, even new_subsubprocesses is cleared and updated
// returns 0 if successful
int update_process_tree(struct process_tree *ptree);

// finds the subprocesses of a given process (children, children of children, etc..)
// the algorithm first finds the process and then, its subprocesses recursively
// you should call build_process_tree() once before using this function
// if pid is spid, consider using get_new_subprocesses() which is much faster, scalable and O(1)
// returns the number of subprocesses found, or -1 if the pid it's not found in the tree
int get_subprocesses(struct process_tree *ptree, int pid, struct list *subprocesses);

// returns special process subprocesses detected by last build_process_tree() or update_process_tree() call
// you should call build_process_tree() once before using this function
int get_new_subprocesses(struct process_tree *ptree, struct list *subprocesses);

// free the heap memory used by a process tree
void cleanup_process_tree(struct process_tree *ptree);

// searches a process given the name of the executable file, or its absolute path
// returns the pid, or 0 if it's not found
int look_for_process_by_name(const char *process);

// searches a process given its pid
// returns the pid, or 0 if it's not found
int look_for_process_by_pid(int pid);

#endif
