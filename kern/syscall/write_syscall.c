/*
 * Write to file system call
 *
 */

#include <syscall.h>
#include <filetable.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <types.h>
#include <vnode.h>
#include <vfs.h>
#include <uio.h>
#include <limits.h>
#include <current.h>
#include <proc.h>

int sys_write(int fd, void *buf, size_t nbytes, ssize_t *bytesWritten){
 	struct file *f;
 	struct uio u;
 	struct iovec i;
 	int result;

 	// get the file struct from the filetable
 	result = filetable_get(curproc->p_ft, fd, &f);
 	if (result) return result;
 	if (!(f->f_flags & O_WRONLY) || !(f->f_flags & O_RDWR)) return EBADF;

 	// set up the uio for writing
 	i.iov_kbase = buf;
	i.iov_len = nbytes;
	u.uio_iov = &i;
	u.uio_iovcnt = 1;
	u.uio_offset = f->f_cursor;
	u.uio_resid = nbytes;
	u.uio_segflg = UIO_USERSPACE;
	u.uio_rw = UIO_WRITE;
	u.uio_space = proc_getas();

	// acquire lock and do the write
	lock_acquire(f->f_lock);
	result = VOP_WRITE(f->f_vn, &u);
	*bytesWritten = u.uio_offset - f->f_cursor;
	f->f_cursor = u.uio_offset;
	lock_release(f->f_lock);

	if (result) return result;

	return 0;
}