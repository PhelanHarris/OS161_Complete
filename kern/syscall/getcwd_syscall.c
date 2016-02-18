/*
 * Get Current Working Directory System Call
 *
 */

#include <syscall.h>
#include <filetable.h>
#include <vfs.h>
#include <uio.h>

int
sys___getcwd (char *buf, size_t buflen)
{
  	struct iovec iov;
  	struct uio uio;
  	int result;
	
	uio_kinit(&iov, &uio, buf, buflen, 0, UIO_READ);

	// Return result
	result = vfs_getcwd(&uio);
	if (result){
		return result;
	}
	return 0;
}