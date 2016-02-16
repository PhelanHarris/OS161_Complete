/*
 * Change Directory System Call
 *
 */

#include <syscall.h>
#include <filetable.h>
#include <vfs.h>

int 
sys_chdir (const char *pathname);
{
	char *path_buffer;
	size_t len;
	int result;

	// Copy filename locally
	path_buffer = (char *) kmalloc(sizeof(char)*PATH_MAX);
	result = copyinstr((const_userptr_t)filename, path_buffer, PATH_MAX, &len);
	if (result) {
		kfree(path_buffer);
		return result;
	}

	result = vfs_chdir(path_buffer);
	kfree(path_buffer);
	return result;
}