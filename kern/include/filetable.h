/*
 * Filetable
 */

#ifndef _FILETABLE_H_
#define _FILETABLE_H_

#include <array.h>
#include <types.h>
#include <vnode.h>
#include <synch.h>

#ifndef FILETABLEINLINE
#define FILETABLEINLINE INLINE
#endif

// Delcares struct filearray
DECLARRAY(file, FILETABLEINLINE);
DEFARRAY(file, FILETABLEINLINE);

struct file {
	vnode *f_vn;
	mode_t f_mode;
	int f_cursor;
	int f_refcount;
	struct lock *f_lock; 
};

int filetable_get(unsigned fd, struct file *f_ret);
int filetable_add(vnode *vn, unsigned *fd_ret);
int filetable_clone(unsigned fd_old, unsigned *fd_new);
void filetable_remove(unsigned fd);

#endif /* _FILETABLE_H_ */
