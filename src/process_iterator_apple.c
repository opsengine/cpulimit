int init_process_iterator(struct process_iterator *it) {
	return 0;
}

int get_next_process(struct process_iterator *it, struct process *p) {
	return -1;
}

int close_process_iterator(struct process_iterator *it) {
	return 0;
}

	// int err;
	// struct kinfo_proc *result = NULL;
	// size_t length;
	// int mib[] = {CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0};

	// /* We start by calling sysctl with result == NULL and length == 0.
	//    That will succeed, and set length to the appropriate length.
	//    We then allocate a buffer of that size and call sysctl again
	//    with that buffer.
	// */
	// length = 0;
	// err = sysctl(mib, 4, NULL, &length, NULL, 0);
	// if (err == -1) {
	// 	err = errno;
	// }
	// if (err == 0) {
	// 	result = malloc(length);
	// 	err = sysctl(mib, 4, result, &length, NULL, 0);
	// 	if (err == -1)
	// 		err = errno;
	// 	if (err == ENOMEM) {
	// 		free(result); /* clean up */
	// 		result = NULL;
	// 	}
	// }

	// i->proclist = result;
	// i->count = err == 0 ? length / sizeof *result : 0;
	// i->c = 0;

// int get_proc_info(struct process *p, pid_t pid) {
// 	int err;
// 	struct kinfo_proc *result = NULL;
// 	size_t length;
// 	int mib[] = {CTL_KERN, KERN_PROC, KERN_PROC_PID, pid};

// 	/* We start by calling sysctl with result == NULL and length == 0.
// 	   That will succeed, and set length to the appropriate length.
// 	   We then allocate a buffer of that size and call sysctl again
// 	   with that buffer.
// 	*/
// 	length = 0;
// 	err = sysctl(mib, 4, NULL, &length, NULL, 0);
// 	if (err == -1) {
// 		err = errno;
// 	}
// 	if (err == 0) {
// 		result = malloc(length);
// 		err = sysctl(mib, 4, result, &length, NULL, 0);
// 		if (err == -1)
// 			err = errno;
// 		if (err == ENOMEM) {
// 			free(result); /* clean up */
// 			result = NULL;
// 		}
// 	}

// 	p->pid = result->kp_proc.p_pid;
// 	p->ppid = result->kp_eproc.e_ppid;
// 	p->starttime = result->kp_proc.p_starttime.tv_sec;
// 	p->last_jiffies = result->kp_proc.p_cpticks;
// 	//p_pctcpu

// 	return 0;
// }
