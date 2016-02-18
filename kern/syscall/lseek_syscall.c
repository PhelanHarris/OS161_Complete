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

int sys_lseek(int fd, off_t pos, int whence, off_t *newCursor){
 	struct file *f;
 	struct stat stat;
 	int result;

 	// get the file struct from the filetable
 	result = filetable_get(curproc->p_ft, fd, &f);
 	if (result) return result;

 	lock_acquire(f->f_lock);
 	if (whence == SEEK_SET){
 		*newCursor = pos;
 	}
 	else if (whence == SEEK_CUR){
 		*newCursor = f->f_cursor + pos;
 	}
 	else if (whence == SEEK_END){
 		VOP_STAT(f->f_vn, &stat);
 		*newCursor = stat.st_size + pos;
 	}
 	else{
 		lock_release(f->f_lock);
 		return EINVAL;
 	}

 	if (*newCursor < 0){
 		return EINVAL;
 	}
 	return 0;
}