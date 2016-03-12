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

void kill_children(struct proc* p);

void
sys__exit(int exitcode)
{	
	// Notify parents (if they are still alive)
	if (curproc->p_parent != NULL) {
		struct proctable_entry *pte = proctable_get(curproc->p_id);
		
		if (pte != NULL) {
			// Broadcast exit code
			pte->pte_exitcode = exitcode;
			cv_broadcast(pte->pte_cv, pte->pte_lock);
		}
	} else {
		// No one loves this child
		proctable_remove(curproc->p_id);
	}

	// Send condolences to the children
	if (curproc->p_children != NULL) {
		struct proc_child *child = curproc->p_children;
		while (child != NULL) {
			child->p_parent = NULL;
			child = child->next;
		}
	}

	// Detach and destroy process
	proc_remthread(curthread);
	proc_destroy(curproc);

	// Zombify
	thread_exit();
}
