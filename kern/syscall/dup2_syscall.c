/*
 * Duplicate2 System Call
 *
 */

#include <syscall.h>
#include <filetable.h>
#include <current.h>

int 
sys_dup2 (int fd_old, int fd_new, int *error)
{
	int result;
	error = 0;
	result = sys_close(fd_new, &error);
	if (result){
		return result;
	}

	result = filetable_clone(curproc->p_ft, (unsigned) fd_old, (unsigned) fd_new);
	if (result){
		error = result;
		return -1;
	}

	return fd_new;
}