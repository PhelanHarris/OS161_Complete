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

int 
sys_open (const char *filename, int flags, int *error)
{
	struct vnode *vn;
	struct file *f;
	unsigned fd;
	char *name_buffer;
	size_t len;
	int result;
	error = 0;

	// Copy filename locally
	name_buffer = (char *) kmalloc(sizeof(char)*PATH_MAX);
	result = copyinstr((const_userptr_t)filename, name_buffer, PATH_MAX, &len);
	if (result) {
		kfree(name_buffer);
		error = result;
		return -1;
	}

	// Open file
	result = vfs_open(name_buffer, flags, 0, &vn);
	kfree(name_buffer);
	if (result){
		error = result;
		return -1;
	}

	// Create filetable entry
	result = filetable_add(curproc->p_ft, vn, &fd);
	if (result) {
		vfs_close(vn);
		error = result;
		return -1;
	}

	return fd;
}