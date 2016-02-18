/*
 * lseek system call
 *
 */

#include <syscall.h>
#include <filetable.h>
#include <kern/errno.h>
#include <kern/seek.h>
#include <types.h>
#include <vnode.h>
#include <vfs.h>
#include <limits.h>
#include <stat.h>
#include <current.h>
#include <proc.h>

off_t sys_lseek(int fd, off_t pos, int whence, int *error){
 	struct file *f;
 	struct stat f_stat;
 	*error = 0;

 	// get the file struct from the filetable
 	*error = filetable_get(curproc->p_ft, fd, &f);
 	if (*error) return -1;

 	lock_acquire(f->f_lock);
 	off_t newCursor;
 	if (whence == SEEK_SET){
 		newCursor = pos;
 	}
 	else if (whence == SEEK_CUR){
 		newCursor = f->f_cursor + pos;
 	}
 	else if (whence == SEEK_END){
 		VOP_STAT(f->f_vn, f_stat);
 		newCursor = f_stat->st_size + pos;
 	}
 	else{
 		lock_release(f->f_lock);
 		*error = EINVAL;
 		return -1;
 	}

 	if (newCursor < 0){
 		*error = EINVAL;
 		return -1;
 	}
 	return newCursor;
}