/*
 * Filetable
 */

#ifndef _FILETABLE_H_
#define _FILETABLE_H_

#include <types.h>
#include <vnode.h>
#include <synch.h>

struct file {
	struct vnode *f_vn;
	mode_t f_mode;
	off_t f_cursor;
	int f_refcount;
	struct lock *f_lock; 
};

struct filetable {
	struct file **ft_arr;
	struct lock *ft_lock;
	unsigned ft_size;
	unsigned ft_lastindex;
};

int filetable_create(struct filetable **ft_ret);
void filetable_destroy(struct filetable *ft);
int filetable_get(struct filetable *ft, unsigned fd, struct file **f_ret);
int filetable_add(struct filetable *ft, struct vnode *vn, unsigned *fd_ret);
int filetable_clone(struct filetable *ft, unsigned fd_old, unsigned fd_new);
int filetable_remove(struct filetable *ft, unsigned fd);

#endif /* _FILETABLE_H_ */
