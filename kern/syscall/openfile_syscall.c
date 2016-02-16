/*
 * Open File System Call
 *
 */

#include <syscall.h>
#include <filetable.h>
#include <kern/errno.h>
#include <types.h>
#include <vnode.h>
#include <vfs.h>
#include <copyinout.h>
#include <limits.h>

int 
sys_open (const char *filename, int flags, int *fd_ret)
{
	struct vnode *vn;
	struct file *f;
	unsigned fd;
	int result;

	char *name_buffer;
	size_t len;

	// Copy filename locally
	name_buffer = (char *) kmalloc(sizeof(char)*PATH_MAX);
	result = copyinstr((const_userptr_t)filename, name_buffer, PATH_MAX, &len);
	if (result) {
		kfree(name_buffer);
		return result;
	}

	// Open file
	result = vfs_open(filename, flags, 0, &vn);
	if (result) {
		kfree(name_buffer);
		return result;
	}

	// Create filetable entry
	result = filetable_add(vn, &fd);
	if (result) {
		kfree(name_buffer);
		vfs_close(vn);
		return result;
	}

	*fd_ret = (int) fd;
	return 0;
}