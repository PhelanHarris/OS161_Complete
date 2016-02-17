/*
 * Change Directory System Call
 *
 */

#include <syscall.h>
#include <filetable.h>
#include <vfs.h>
#include <limits.h>
#include <copyinout.h>
#include <current.h>

int 
sys_chdir (const char *pathname)
{
	char *path_buffer;
	size_t len;
	int result;

	// Copy pathname locally
	path_buffer = (char *) kmalloc(sizeof(char)*PATH_MAX);
	result = copyinstr((const_userptr_t)pathname, path_buffer, PATH_MAX, &len);
	if (result) {
		kfree(path_buffer);
		return result;
	}

	result = vfs_chdir(path_buffer);
	kfree(path_buffer);
	return result;
}