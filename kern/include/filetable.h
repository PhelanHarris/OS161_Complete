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
	off_t f_cursor;
	int f_refcount;
	struct lock *f_lock; 
};

struct filetable {
	struct filearray ft_arr;
	struct lock *ft_lock;
};

int filetable_init(struct filetable *ft);
int filetable_destroy(struct filetable *ft);
int filetable_get(struct filetable *ft, unsigned fd, struct file *f_ret);
int filetable_add(struct filetable *ft, vnode *vn, unsigned *fd_ret);
int filetable_clone(struct filetable *ft, unsigned fd_old, unsigned *fd_new);
int filetable_remove(struct filetable *ft, unsigned fd);

#endif /* _FILETABLE_H_ */
