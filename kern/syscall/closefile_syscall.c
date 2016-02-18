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
sys_close (int fd)
{
	struct file *f;
	int result;

	// Get filetable entry
	kprintf("Curproc: %s", curproc->p_name);
	result = filetable_get(curproc->p_ft, (unsigned) fd, &f);
	if (result) {
		return result;
	}

	// Close at vfs level
	lock_acquire(f->f_lock);
	vfs_close(f->f_vn);
	lock_release(f->f_lock);

	// Remove filetable entry
	filetable_remove(curproc->p_ft, (unsigned) fd);
	return 0;
}