/*
 * Read from file system call
 *
 */

#include <syscall.h>
#include <filetable.h>
#include <kern/errno.h>
#include <types.h>
#include <vnode.h>
#include <vfs.h>
#include <uio.h>
#include <copyinout.h>
#include <limits.h>
#include <current.h>
#include <proc.h>

ssize_t sys_read(int fd, void *buf, size_t buflen, int *error){
 	struct file *f;
 	struct uio u;
 	struct iovec i;
 	*error = 0;

 	// get the file struct from the filetable
 	*error = filetable_get(&curproc->p_ft, fd, &f);
 	if (*error) return -1;

 	// set up the uio for reading
 	i.iov_kbase = buf;
	i.iov_len = buflen;
	u.uio_iov = &i;
	u.uio_iovcnt = 1;
	u.uio_offset = f->f_cursor;
	u.uio_resid = buflen;
	u.uio_segflg = UIO_USERSPACE;
	u.uio_rw = UIO_READ;
	u.uio_space = NULL;

	// acquire the lock and do the read
	lock_acquire(f->f_lock);
	*error = VOP_READ(f->f_vn, &u);
	ssize_t bytesRead = u.uio_offset - f->f_cursor;
	f->f_cursor = u.uio_offset;
	lock_release(f->f_lock);

	if (*error) return -1;

	return bytesRead;

}