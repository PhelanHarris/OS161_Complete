/*
 * Write to file system call
 *
 */

#include <syscall.h>
#include <filetable.h>
#include <kern/errno.h>
#include <types.h>
#include <vnode.h>
#include <vfs.h>
#include <uio.h>
#include <limits.h>
#include <current.h>
#include <proc.h>

ssize_t sys_write(int fd, void *buf, size_t nbytes, int *error){
 	struct file *f;
 	struct uio u;
 	struct iovec i;
 	*error = 0;

 	// get the file struct from the filetable
 	*error = filetable_get(curproc->p_ft, fd, &f);
 	if (*error) return -1;

 	// set up the uio for writing
 	i.iov_kbase = buf;
	i.iov_len = nbytes;
	u.uio_iov = &i;
	u.uio_iovcnt = 1;
	u.uio_offset = f->f_cursor;
	u.uio_resid = nbytes;
	u.uio_segflg = UIO_USERSPACE;
	u.uio_rw = UIO_WRITE;
	u.uio_space = NULL;

	// acquire lock and do the write
	lock_acquire(f->f_lock);
	*error = VOP_WRITE(f->f_vn, &u);
	ssize_t bytesWritten = u.uio_offset - f->f_cursor;
	f->f_cursor = u.uio_offset;
	lock_release(f->f_lock);

	if (*error) return -1;

	return bytesWritten;
}