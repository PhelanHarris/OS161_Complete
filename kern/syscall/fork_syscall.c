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
	
	// Create new process
	struct proc *child = proc_create_child("child process");
	if (child == NULL)
		return ENOMEM;

	// Copy address space
	ret = as_copy(curproc->p_addrspace, &child->p_addrspace);
	if (ret) {
		proc_destroy(child);		
		proctable_remove(child->p_id);
		return ret;
	}

	// Copy filetable
	ret = filetable_clone(curproc->p_ft, child->p_ft);
	if (ret) {
		proc_destroy(child);
		proctable_remove(child->p_id);
		return ret;
	}

	// Copy trapframe
	struct trapframe *child_tf = kmalloc(sizeof(struct trapframe));
	if (child_tf == NULL) {
		proc_destroy(child);
		proctable_remove(child->p_id);
		return ENOMEM;
	}
	memcpy(child_tf, tf, sizeof(struct trapframe));

	// Fork new thread and attach to new process
	ret = thread_fork("child thread", child, enter_forked_process, (void*)child_tf, 0);
	if (ret) {
		proc_destroy(child);
		proctable_remove(child->p_id);
		return ret;
	}

	*pid_ret = child->p_id;
	return 0;
}
