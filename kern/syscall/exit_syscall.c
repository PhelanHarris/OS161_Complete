/*
 * Exit system call
 *
 */

#include <types.h>
#include <array.h>
#include <proc.h>
#include <syscall.h>
#include <proctable.h>
#include <synch.h>
#include <current.h>

void
sys__exit(int exitcode)
{	
	struct proctable_entry *pte = proctable_get(curproc->p_id);
	KASSERT(pte != NULL); // a bit of a problem if we're on a null process

	// Broadcast exit code
	pte->pte_exitcode = exitcode;
	cv_broadcast(pte->pte_cv, pte->pte_lock);

	// KILL ALL THE CHILDREN
	int i;
	for (i = curproc->p_children->arr.num - 1; i >= 0; i--) {
		pid_t child_pid = (pid_t) pidarray_get(curproc->p_children, i);
		struct proctable_entry *child = proctable_get(child_pid);
		proctable_remove(child_pid);
		proc_destroy(child->pte_p);
		pidarray_remove(curproc->p_children, i);
	}

	// Destroy process
	proc_destroy(pte->pte_p);
}
