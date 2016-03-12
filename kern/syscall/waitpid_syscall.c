/*
 * Wait for PID system call
 *
 */

#include <types.h>
#include <syscall.h>
#include <proctable.h>
#include <current.h>
#include <proc.h>
#include <synch.h>
#include <copyinout.h> 
#include <kern/errno.h>

int
sys_waitpid(pid_t pid, userptr_t status, int options)
{	
	(void) options;
	int child_status;
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

	// Check if process has not already exited
	if (child_pte->pte_exitcode == -1) {
		// Wait for child to exit
		while (child_pte->pte_exitcode == -1) {
			cv_wait(child_pte->pte_cv, child_pte->pte_lock);
		}
	}

	if (status != NULL) {
		child_status = child_pte->pte_exitcode;
		copyout(&child_status, status, sizeof(int));
	}

	lock_release(child_pte->pte_lock);
	return 0;
}
