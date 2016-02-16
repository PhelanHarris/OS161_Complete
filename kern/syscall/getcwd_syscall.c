/*
 * Get Current Working Directory System Call
 *
 */

#include <syscall.h>
#include <filetable.h>
#include <vfs.h>
#include <uio.h>

int
sys__getcwd (char *buf, size_t buflen);
{
  	struct iovec iov;
  	struct uio uio;
  	int result;

	// Create uio for data transfer
	void uio_kinit(&iov, &uio, buf, buflen, 0 UIO_READ);

	// Return result
	return vfs_getcwd(uio);
}