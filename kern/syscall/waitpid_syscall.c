/*
 * Wait for PID system call
 *
 */

#include <syscall.h>
#include <proctable.h>
#include <current.h>
#include <proc.h>
#include <synch.h>
#include <types.h>
#include <kern/errno.h>

int
sys_waitpid(pid_t pid, int *status, int options)
{	
	(void) options;

	int i;
	bool found = false;
	struct proctable_entry *child_pte = proctable_get(pid);
	if (child_pte == NULL) {
		return ESRCH;
	}

	// Check if proc is a child of curproc
	for (i = 0; i < curproc->p_children->num; i++) {
		pid_t child_pid = pidarray_get(curproc->p_children, i);
		if (child_pid == pid) {
			found = true;
			break;
		}
	}
	if (!found) {
		return ECHILD;
	}

	lock_acquire(child_pte->pte_lock);

	// Check if process has already exited
	if (pte->pte_exitcode != -1) {
		if (status != NULL) {
			*status = pte->pte_exitcode;
		}
		lock_release(child_pte->pte_lock);
		return 0;
	}

	// Wait for child to exit
	while (pte->pte_exitcode == -1) {
		cv_wait(pte->pte_cv, pte->pte_lock);
	}

	if (status != NULL) {
		*status = pte->pte_exitcode;
	}
	lock_release(child_pte->pte_lock);
	return 0;
}
