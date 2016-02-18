/*
 * Duplicate2 System Call
 *
 */

#include <syscall.h>
#include <filetable.h>
#include <current.h>
#include <proc.h>

int 
sys_dup2 (int fd_old, int fd_new, int *error)
{
	*error = 0;
	*error = sys_close(fd_new);
	if (*error){
		return -1;
	}

	
	*error = filetable_clone(curproc->p_ft, (unsigned) fd_old, (unsigned) fd_new);
	if (*error){
		return -1;
	}

	return fd_new;
}