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
	struct proc *child;
	struct trapframe *child_tf;
	int result;

	// Create new process
	child = proc_create_child("child process");
	if (child == NULL) {	
		return ENOMEM;
	}

	// Copy filetable
	result = filetable_clone(curproc->p_ft, child->p_ft);
	if (result) {
		proctable_remove(child->p_id);
		proc_destroy(child);
		return result;
	}

	// Malloc address space
	child->p_addrspace = (struct addrspace *) kmalloc(sizeof(struct addrspace));
	if (child->p_addrspace == NULL) {
		proctable_remove(child->p_id);
		proc_destroy(child);
		return ENOMEM;
	}

	// Copy address space
	result = as_copy(curproc->p_addrspace, &child->p_addrspace);
	if (result) {
		proctable_remove(child->p_id);
		proc_destroy(child);
		return result;
	}
	
	// Copy trapframe
	child_tf = (struct trapframe *) kmalloc(sizeof(struct trapframe));
	if (child_tf == NULL) {
		proctable_remove(child->p_id);
		proc_destroy(child);
		return ENOMEM;
	}
	memcpy(child_tf, tf, sizeof(struct trapframe));

	// Fork new thread and attach to new process
	result = thread_fork("child thread", child, enter_forked_process, (void*)child_tf, 0);
	if (result) {
		kfree(child_tf);
		proctable_remove(child->p_id);
		proc_destroy(child);
		return result;
	}

	// Pass back new pid
	*pid_ret = child->p_id;

	return 0;
}
