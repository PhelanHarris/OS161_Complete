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

	// PTE should never be null if this proc's thread still exists at this point
	KASSERT(pte != NULL);

	// Update exit code
	lock_acquire(pte->pte_lock);
	pte->pte_running = false;
	pte->pte_exitcode = exitcode;

	// Broadcast
	cv_broadcast(pte->pte_cv, pte->pte_lock);

	// Decrement refcount of children
	if (curproc->p_children != NULL) {
		struct proc_child *child = curproc->p_children;
		while (child != NULL) {
			proctable_remove(child->child_pid);
			child = child->next;
		}
	}

	// Decrement own refcount
	bool allAlone = proctable_remove(curproc->p_id);
	
	// Detach and destroy process
	proc_remthread(curthread);
	proc_destroy(curproc);

	// Release lock if not all alone (otherwise, lock has been destroyed)
	if (!allAlone) 
		lock_release(pte->pte_lock);

	// Zombify
	thread_exit();
}
