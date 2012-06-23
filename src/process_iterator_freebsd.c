#include <sys/sysctl.h>
#include <sys/user.h>
#include <fcntl.h>
#include <paths.h>

int init_process_iterator(struct process_iterator *it, struct process_filter *filter) {
	char errbuf[_POSIX2_LINE_MAX];
	it->i = 0;
	/* Open the kvm interface, get a descriptor */
	if ((it->kd = kvm_openfiles(NULL, _PATH_DEVNULL, NULL, O_RDONLY, errbuf)) == NULL) {
		fprintf(stderr, "kvm_open: %s\n", errbuf);
		return -1;
	}
	/* Get the list of processes. */
	if ((it->procs = kvm_getprocs(it->kd, KERN_PROC_PROC, 0, &it->count)) == NULL) {
		kvm_close(it->kd);
		fprintf(stderr, "kvm_getprocs: %s\n", kvm_geterr(it->kd));
		return -1;
	}
	it->filter = filter;
	return 0;
}

static void kproc2proc(kvm_t *kd, struct kinfo_proc *kproc, struct process *proc)
{
	proc->pid = kproc->ki_pid;
	proc->ppid = kproc->ki_ppid;
	proc->cputime = kproc->ki_runtime / 1000;
	proc->starttime = kproc->ki_start.tv_sec;
	char **args = kvm_getargv(kd, kproc, sizeof(proc->command));
	if (args != NULL)
	{
		memcpy(proc->command, args[0], strlen(args[0]) + 1);
	}
}

static int get_single_process(kvm_t *kd, pid_t pid, struct process *process)
{
	int count;
	struct kinfo_proc *kproc = kvm_getprocs(kd, KERN_PROC_PID, pid, &count);
	if (count == 0 || kproc == NULL)
	{
		fprintf(stderr, "kvm_getprocs: %s\n", kvm_geterr(kd));
		return -1;
	}
	kproc2proc(kd, kproc, process);
	return 0;
}

int get_next_process(struct process_iterator *it, struct process *p) {
	if (it->i == it->count)
	{
		return -1;
	}
	if (it->filter->pid > 0 && !it->filter->include_children)
	{
		get_single_process(it->kd, it->filter->pid, p);
		it->i = it->count = 1;
		return 0;
	}
	while (it->i < it->count)
	{
		struct kinfo_proc *kproc = &(it->procs[it->i]);
		if (kproc->ki_flag & P_SYSTEM)
		{
			// skip system processes
			it->i++;
			continue;
		}
		if (it->filter->pid > 0 && it->filter->include_children)
		{
			kproc2proc(it->kd, kproc, p);
			it->i++;
			if (p->pid != it->filter->pid && p->ppid != it->filter->pid)
				continue;
			return 0;
		}
		else if (it->filter->pid == 0)
		{
			kproc2proc(it->kd, kproc, p);
			it->i++;
			return 0;
		}
	}
	return -1;
}

int close_process_iterator(struct process_iterator *it) {
	if (kvm_close(it->kd) == -1) {
		fprintf(stderr, "kvm_getprocs: %s\n", kvm_geterr(it->kd));
		return -1;
	}
	return 0;
}

