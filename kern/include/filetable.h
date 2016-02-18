/*
 * Filetable
 */

#ifndef _FILETABLE_H_
#define _FILETABLE_H_

#include <types.h>
#include <vnode.h>
#include <synch.h>
#include <limits.h>


struct file {
	struct vnode *f_vn;
	int f_flags;
	off_t f_cursor;
	int f_refcount;
	struct lock *f_lock; 
};

struct filetable {
	struct file **ft_arr;
	struct lock *ft_lock;
	unsigned ft_size;
};

int filetable_init(struct filetable *ft);
void filetable_destroy(struct filetable *ft);
int filetable_get(struct filetable *ft, unsigned fd, struct file **f_ret);
int filetable_add(struct filetable *ft, struct vnode *vn, int flags, unsigned *fd_ret);
int filetable_clone(struct filetable *ft, unsigned fd_old, unsigned fd_new);
int filetable_remove(struct filetable *ft, unsigned fd);

#endif /* _FILETABLE_H_ */
