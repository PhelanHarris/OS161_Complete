/*
 * Change Directory System Call
 *
 */

#include <types.h>
#include <limits.h>
#include <copyinout.h>
#include <kern/errno.h>
#include <syscall.h>
#include <filetable.h>
#include <vfs.h>
#include <current.h>

int 
sys_chdir(const char *pathname)
{
	char *path_buffer;
	size_t len;
	int result;

	// Allocate space for pathname
	path_buffer = (char *) kmalloc(sizeof(char)*PATH_MAX);
	if (path_buffer == NULL) {
		return ENOMEM;
	}

	// Copy pathname locally
	result = copyinstr((const_userptr_t)pathname, path_buffer, PATH_MAX, &len);
	if (result) {
		kfree(path_buffer);
		return result;
	}

	result = vfs_chdir(path_buffer);
	kfree(path_buffer);
	return result;
}