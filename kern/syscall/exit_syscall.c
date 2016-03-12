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
	struct proc *p = curproc;
	pid_t pid = p->p_id;
	struct proctable_entry *pte = proctable_get(pid);

	// PTE should never be null if this proc's thread still exists at this point
	KASSERT(pte != NULL);

	// Update exit code
	lock_acquire(pte->pte_lock);
	pte->pte_running = false;
	pte->pte_exitcode = exitcode;

	// Broadcast
	cv_broadcast(pte->pte_cv, pte->pte_lock);

	// Decrement refcount of children
	if (p->p_children != NULL) {
		struct proc_child *child = p->p_children;
		while (child != NULL) {
			proctable_remove(child->child_pid);
			child = child->next;
		}
	}

	// Detach and destroy process
	proc_remthread(curthread);
	proc_destroy(p);
	pte->pte_p = NULL;

	// Decrement own refcount (and release lock if it wasn't already destroyed)
	if (!proctable_remove(pid))
		lock_release(pte->pte_lock);

	// Zombify
	thread_exit();
}
