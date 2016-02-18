/*
 * Duplicate2 System Call
 *
 */

#include <syscall.h>
#include <filetable.h>
#include <current.h>
#include <proc.h>

int 
sys_dup2 (int fd_old, int fd_new)
{
	int result;
	result = sys_close(fd_new);
	if (result) return result;

	
	result = filetable_clone(curproc->p_ft, (unsigned) fd_old, (unsigned) fd_new);
	if (result) return result;

	return 0;
}