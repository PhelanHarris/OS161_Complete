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

	unsigned i;
	bool found = false;
	struct proctable_entry *child_pte = proctable_get(pid);
	if (child_pte == NULL) {
		return ESRCH;
	}

	// Check if proc is a child of curproc
	for (i = 0; i < pidarray_num(curproc->p_children); i++) {
		pid_t child_pid = (pid_t)pidarray_get(curproc->p_children, i);
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
	if (child_pte->pte_exitcode != -1) {
		if (status != NULL) {
			*status = child_pte->pte_exitcode;
		}
		lock_release(child_pte->pte_lock);
		return 0;
	}

	// Wait for child to exit
	while (child_pte->pte_exitcode == -1) {
		cv_wait(child_pte->pte_cv, child_pte->pte_lock);
	}

	if (status != NULL) {
		*status = child_pte->pte_exitcode;
	}
	lock_release(child_pte->pte_lock);
	return 0;
}
