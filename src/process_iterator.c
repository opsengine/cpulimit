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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "process_iterator.h"

/* See this link to port to other systems: http://www.steve.org.uk/Reference/Unix/faq_8.html#SEC85 */

int is_child_of(pid_t child_pid, pid_t parent_pid)
{
	if (child_pid <= 0 || parent_pid <= 0)
		return 0;
	while (child_pid > 1 && child_pid != parent_pid)
	{
		child_pid = getppid_of(child_pid);
	}
	return child_pid == parent_pid;
}

#if defined(__linux__)

#include "process_iterator_linux.c"

#elif defined(__FreeBSD__)

#include "process_iterator_freebsd.c"

#elif defined(__APPLE__)

#include "process_iterator_apple.c"

#else

#error Platform not supported

#endif
