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
	KASSERT(pte != NULL); // a bit of a problem if we're on a null process

	// Broadcast exit code
	pte->pte_exitcode = exitcode;
	cv_broadcast(pte->pte_cv, pte->pte_lock);

	// KILL ALL THE CHILDREN
	kill_children(curproc);

	// Destroy process
	proc_destroy(pte->pte_p);
}

void 
kill_children(struct proc* p)
{
	struct proc_child* child = p->p_children;
	while (child != NULL) {
		struct proctable_entry *child_pte = proctable_get(child->child_pid);
		if (child_pte->pte_p->p_children != NULL){
			kill_children(child_pte->pte_p);
		}
		proctable_remove(child->child_pid);
		proc_destroy(child_pte->pte_p);
		child = child->next;
		kfree(p->p_children);
		p->p_children = child;
	}
}
