/*
 * Exit system call
 *
 */

#include <syscall.h>
#include <proc.h>
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
	
	// Destroy process
	proc_destroy(pte->pte_p);
}
