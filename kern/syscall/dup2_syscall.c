/*
 * Duplicate2 System Call
 *
 */

#include <syscall.h>
#include <filetable.h>

int 
sys_dup2 (int fd_old, int fd_new)
{
	sys_close(fd_new);
	filetable_clone(curproc->ft, (unsigned) fd_old, (unsigned) fd_new);
	return 0;
}