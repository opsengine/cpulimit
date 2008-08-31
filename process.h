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

#ifndef __PROCESS_H

#define __PROCESS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

//kernel time resolution timer interrupt frequency in Hertz
//#define HZ sysconf(_SC_CLK_TCK)

//HZ detection, from openssl code
#ifndef HZ
# if defined(_SC_CLK_TCK) \
     && (!defined(OPENSSL_SYS_VMS) || __CTRL_VER >= 70000000)
#  define HZ ((double)sysconf(_SC_CLK_TCK))
# else
#  ifndef CLK_TCK
#   ifndef _BSD_CLK_TCK_ /* FreeBSD hack */
#    define HZ  100.0
#   else /* _BSD_CLK_TCK_ */
#    define HZ ((double)_BSD_CLK_TCK_)
#   endif
#  else /* CLK_TCK */
#   define HZ ((double)CLK_TCK)
#  endif
# endif
#endif


//process descriptor
struct process_history {
	//the PID of the process
	pid_t pid;
	//timestamp when last_j and cpu_usage was calculated
	struct timeval last_sample;
	//total number of jiffies used by the process at time last_sample
	int last_jiffies;
	//cpu usage estimation (value in range 0-1)
	double cpu_usage;
#ifdef __linux__
	//preallocate buffers for performance
	//name of /proc/PID/stat file
	char stat_file[20];
	//read buffer for /proc filesystem
	char buffer[1024];
#endif
};

int process_init(struct process_history *proc, pid_t pid);

int process_monitor(struct process_history *proc);

int process_close(struct process_history *proc);

#endif
