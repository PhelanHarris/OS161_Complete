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
	struct proctable_entry *pte = proctable_get(curproc->p_id);
	if (pte == NULL) {
		thread_exit();
	}
	KASSERT(pte != NULL); // a bit of a problem if we're on a null process

	// KILL ALL THE CHILDREN
	kill_children(curproc);

	// Broadcast exit code
	pte->pte_exitcode = exitcode;
	cv_broadcast(pte->pte_cv, pte->pte_lock);

	// Destroy process
	//proc_destroy(pte->pte_p);
	thread_exit();
}

void 
kill_children(struct proc* p)
{
	struct proc_child* child = p->p_children;
	while (child != NULL) {
		struct proctable_entry *child_pte = proctable_get(child->child_pid);
		proctable_remove(child->child_pid);

		// Kill its children
		if (child_pte->pte_p->p_children != NULL){
			kill_children(child_pte->pte_p);
		}
		proc_destroy(child_pte->pte_p);
		child = child->next;
		kfree(p->p_children);
		p->p_children = child;
	}
}
