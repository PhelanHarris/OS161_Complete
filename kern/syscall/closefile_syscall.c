/*
 * Close File System Call
 *
 */

#include <syscall.h>
#include <filetable.h>
#include <vfs.h>
#include <synch.h>
#include <current.h>
#include <proc.h>

int 
sys_close(int fd)
{
	int result = filetable_remove(curproc->p_ft, (unsigned) fd);
	return result ? result : 0;
}