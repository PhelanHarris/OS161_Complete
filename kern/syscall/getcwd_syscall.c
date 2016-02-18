/*
 * Get Current Working Directory System Call
 *
 */

#include <syscall.h>
#include <filetable.h>
#include <vfs.h>
#include <uio.h>

int
sys___getcwd (char *buf, size_t buflen, int *error)
{
  	struct iovec iov;
  	struct uio uio;
  	int result;

	// Create uio for data transfer
	iov->iov_kbase = buf;
	iov->iov_len = buflen;
	uio->uio_iov = iov;
	uio->uio_iovcnt = 1;
	uio->uio_offset = 0;
	uio->uio_resid = buflen;
	uio->uio_segflg = UIO_SYSSPACE;
	uio->uio_rw = UIO_READ;
	uio->uio_space = NULL;
	
	//void uio_kinit(&iov, &uio, buf, buflen, 0, UIO_READ);

	// Return result
	result = vfs_getcwd(uio);
	if (result){
		error = result;
		return -1;
	}
	return result;
}