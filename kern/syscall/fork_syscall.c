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
	struct proc *child = proc_create_runprogram("child process");
	if (child == NULL) // need to check for if max process count exceeded here also
		return ENOMEM;

	// Add child to parent's children
	struct proc_child *child_entry = kmalloc(sizeof(struct proc_child));
	child_entry->child_pid = child->p_id;
	child_entry->next = curproc->p_children;
	curproc->p_children = child_entry;

	// The parent loves its new child
	struct proctable_entry *child_pte = proctable_get(child->p_id);
	KASSERT(child_pte != NULL);
	child_pte->pte_refcount++; // refcount should be 2

	// Copy address space
	ret = as_copy(curproc->p_addrspace, &child->p_addrspace);
	if (ret)
		return ret;

	// Copy filetable
	ret = filetable_clone(curproc->p_ft, child->p_ft);
	if (ret)
		return ret;

	// Copy trapframe
	struct trapframe *child_tf = kmalloc(sizeof(struct trapframe));
	memcpy(child_tf, tf, sizeof(struct trapframe));

	// Fork new thread and attach to new process
	ret = thread_fork("child thread", child, enter_forked_process, (void*)child_tf, 0);
	if (ret)
		return ret;

	*pid_ret = child->p_id;
	return 0;
}
