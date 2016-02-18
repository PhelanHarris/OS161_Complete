/*
 * Open File System Call
 *
 */

#include <syscall.h>
#include <filetable.h>
#include <vfs.h>
#include <types.h>
#include <vnode.h>
#include <copyinout.h>
#include <limits.h>
#include <kern/errno.h>
#include <current.h>
#include <proc.h>

int 
sys_open (const char *filename, int flags, int *fd_ret)
{
	struct vnode *vn;
	char *name_buffer;
	size_t len;
	int result;

	// Copy filename locally
	name_buffer = (char *) kmalloc(sizeof(char)*PATH_MAX);
	result = copyinstr((const_userptr_t)filename, name_buffer, PATH_MAX, &len);
	if (result) {
		kfree(name_buffer);
		return result;
	}

	// Open file
	result = vfs_open(name_buffer, flags, 0, &vn);
	kfree(name_buffer);
	if (result) {
		vfs_close(vn);
		return result;
	}

	// Create filetable entry
	result = filetable_add(curproc->p_ft, vn, flags, (unsigned *) fd_ret);
	if (result) {
		vfs_close(vn);
		return result;
	}

	return 0;
}