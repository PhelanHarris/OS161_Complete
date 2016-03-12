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

	bool found = false;
	struct proctable_entry *child_pte = proctable_get(pid);
	if (child_pte == NULL) {
		return ESRCH;
	}

	// Check if proc is a child of curproc
	struct proc_child *child = curproc->p_children;
	while (child != NULL){
		if (child->child_pid == pid) {
			found = true;
			break;
		}
		child = child->next;
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
