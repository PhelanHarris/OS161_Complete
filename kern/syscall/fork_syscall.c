/*
 * Fork system call
 *
 */

#include <types.h>
#include <proc.h>
#include <current.h>
#include <syscall.h>
#include <filetable.h>
#include <proctable.h>
#include <mips/trapframe.h>
#include <kern/errno.h>
#include <addrspace.h>

int
sys_fork(struct trapframe *tf, pid_t *pid_ret)
{
	int ret;
	
	// create new process
	struct proc *child_proc = proc_create_runprogram("child process");
	if (child_proc == NULL) // need to check for if max process count exceeded here also
		return ENOMEM;

	// copy address space
	ret = as_copy(curproc->p_addrspace, &child_proc->p_addrspace);
	if (ret)
		return ret;

	// copy filetable
	ret = filetable_clone(curproc->p_ft, child_proc->p_ft);
	if (ret)
		return ret;

	// copy trapframe
	struct trapframe *child_tf;
	child_tf = kmalloc(sizeof(*child_tf));
	if (child_tf == NULL)
		return ENOMEM;
	*child_tf = *tf;

	// fork new thread and attach to new process
	ret = thread_fork("child thread", child_proc, enter_forked_process, (void*)child_tf, 0);
	if (ret)
		return ret;

	*pid_ret = 0;
	return 0;
}
