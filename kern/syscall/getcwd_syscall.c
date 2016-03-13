/*
 * Get Current Working Directory System Call
 *
 */

#include <types.h>
#include <syscall.h>
#include <filetable.h>
#include <vfs.h>
#include <uio.h>
#include <kern/errno.h>
#include <copyinout.h>

int
sys___getcwd(char *buf, size_t buflen, int *len)
{
	if (buf == NULL) {
		return EFAULT;
	}

  	struct iovec iov;
  	struct uio uio;
  	char *kbuf = (char *) kmalloc(sizeof(*kbuf)*buflen);
  	int result;

  	if (kbuf == NULL) {
  		return ENOMEM;
  	}
	
	uio_kinit(&iov, &uio, kbuf, buflen, 0, UIO_READ);

	// Get cwd
	result = vfs_getcwd(&uio);
	if (result) {
		kfree(kbuf);
		return result;
	}

	// Pass back offset
	*len = uio.uio_offset;

	// Copy into user-space buffer
	result = copyout(kbuf, (userptr_t) buf, buflen);
	kfree(kbuf);
	return result;
}