/*
 * Duplicate2 System Call
 *
 */

#include <syscall.h>
#include <filetable.h>
#include <vfs.h>
#include <synch.h>

int 
sys_dup2 (int fd_old, int fd_new)
{
	sys_close(fd_new);
	filetable_clone((unsigned) fd_old, (unsigned) fd_new);
	return 0;
}