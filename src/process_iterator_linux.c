/**
 *
 * cpulimit - a CPU limiter for Linux
 *
 * Copyright (C) 2005-2012, by:  Angelo Marletta <angelo dot marletta at gmail dot com>
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

#include <sys/vfs.h>

static int check_proc(void)
{
	struct statfs mnt;
	if (statfs("/proc", &mnt) < 0)
		return 0;
	if (mnt.f_type != 0x9fa0)
		return 0;
	return 1;
}

int init_process_iterator(struct process_iterator *it, struct process_filter *filter)
{
	if (!check_proc())
	{
		fprintf(stderr, "procfs is not mounted!\nAborting\n");
		exit(-2);
	}
	/* open a directory stream to /proc directory */
	if ((it->dip = opendir("/proc")) == NULL)
	{
		perror("opendir");
		return -1;
	}
	it->filter = filter;
	return 0;
}

static int read_process_info(pid_t pid, struct process *p)
{
	char statfile[32], exefile[32], state;
	long utime, stime, ppid;
	FILE *fd;

	p->pid = pid;

	/* read command line */
	sprintf(exefile, "/proc/%ld/cmdline", (long)p->pid);
	fd = fopen(exefile, "r");
	if (fd == NULL)
		goto error_out1;
	if (fgets(p->command, sizeof(p->command), fd) == NULL)
	{
		fclose(fd);
		goto error_out1;
	}
	fclose(fd);

	/* read stat file */
	sprintf(statfile, "/proc/%ld/stat", (long)p->pid);
	fd = fopen(statfile, "r");
	if (fd == NULL)
		goto error_out2;

	if (fscanf(fd, "%*d (%*[^)]) %c %ld %*d %*d %*d %*d %*d %*d %*d %*d %*d %ld %ld",
			   &state, &ppid, &utime, &stime) != 4 ||
		state == 'Z' || state == 'X')
	{
		fclose(fd);
		goto error_out2;
	}
	fclose(fd);
	p->ppid = (pid_t)ppid;
	p->cputime = (int)((utime + stime) * 1000 / HZ);
	return 0;

error_out1:
	p->command[0] = '\0';
error_out2:
	return -1;
}

static pid_t getppid_of(pid_t pid)
{
	char statfile[32];
	FILE *fd;
	long ppid = -1;
	sprintf(statfile, "/proc/%ld/stat", (long)pid);
	if ((fd = fopen(statfile, "r")) != NULL)
	{
		fscanf(fd, "%*d (%*[^)]) %*c %ld", &ppid);
		fclose(fd);
	}
	return (pid_t)ppid;
}

static int is_child_of(pid_t child_pid, pid_t parent_pid)
{
	pid_t ppid = child_pid;
	while (ppid > 1 && ppid != parent_pid)
	{
		ppid = getppid_of(ppid);
	}
	return ppid == parent_pid;
}

int get_next_process(struct process_iterator *it, struct process *p)
{
	struct dirent *dit = NULL;

	if (it->dip == NULL)
	{
		/* end of processes */
		return -1;
	}
	if (it->filter->pid != 0 && !it->filter->include_children)
	{
		int ret = read_process_info(it->filter->pid, p);
		closedir(it->dip);
		it->dip = NULL;
		if (ret != 0)
			return -1;
		return 0;
	}

	/* read in from /proc and seek for process dirs */
	while ((dit = readdir(it->dip)) != NULL)
	{
		if (strtok(dit->d_name, "0123456789") != NULL)
			continue;
		p->pid = (pid_t)atol(dit->d_name);
		if (it->filter->pid != 0 &&
			it->filter->pid != p->pid &&
			!is_child_of(p->pid, it->filter->pid))
			continue;
		read_process_info(p->pid, p);
		break;
	}
	if (dit == NULL)
	{
		/* end of processes */
		closedir(it->dip);
		it->dip = NULL;
		return -1;
	}
	return 0;
}

int close_process_iterator(struct process_iterator *it)
{
	if (it->dip != NULL && closedir(it->dip) == -1)
	{
		perror("closedir");
		return 1;
	}
	it->dip = NULL;
	return 0;
}
