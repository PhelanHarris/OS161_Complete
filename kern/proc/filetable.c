/*
 * Filetable Implementation
 */

#define FILETABLEINLINE

#include <filetable.h>
#include <vnode.h>
#include <spinlock.h>
#include <kern/errno.h>

struct filearray *filetable = NULL;
struct lock *filetable_lock = NULL;

void
filetable_init()
{	
	if (filetable_lock == NULL)
		filetable_lock = lock_create("filetable");

	if (filetable == NULL)
		filetable = filearray_create();
}

int
filetable_get(unsigned fd, struct file **f_ret)
{
	f_ret = NULL;

	if (fd > filetable->arr->num) return EBADF;
	*f_ret = filearray_get(filetable, fd);

	if (f_ret == NULL) return EBADF;
	return 0;
}

/*
 * Creates a new file object based on the provided vnode. On success, returns 0
 * and returns the file descriptor in fd_ret. Otherwise, may return the error
 * code and fd_ret will be null.
 */
int
filetable_add(vnode *vn, unsigned *fd_ret)
{
	int ret;
	struct file *f;

	// Make atomic
	lock_acquire(filetable_lock);

	// TODO: check if not at max number of files (return EMFILE/ENFILE)
	
	// Create new file object & add to filetable
	f = kmalloc(sizeof(struct file));
	ret = filearray_add(filetable, f, fd_ret);
	if (ret) {
		kfree(k);
		return ret;
	}

	// Populate
	f->f_vn = vn;
	f->f_mode = 0;
	f->f_cursor = 0;
	f->f_refcount = 1;
	f->f_lock = lock_create("filelock"); // name is not important

	lock_release(filetable_lock);
	return ret;
}

/*
 * Clones a file descriptor and returns it in fd_new. Returns 0 on success,
 * or the error code.
 */
int
filetable_clone(unsigned fd_old, unsigned *fd_new) {
	int ret;
	struct file *f = NULL;

	// Make atomic
	lock_acquire(filetable_lock);

	f = filearray_get(filetable, fd);
	if (f == NULL) return EBADF;

	// TODO: if max number of files return EMFILE (proc) / ENFILE (sys-wide)
	
	f->refcount++;	
	ret = filearray_add(filetable, f, fd_new);

	lock_release(filetable_lock);
	return ret;
}

/*
 * Removes a file descriptor and the file object if no other fd points to it.
 */
void
filetable_remove(unsigned fd) {
	struct file *f = NULL;

	// Make atomic
	lock_acquire(filetable_lock);

	f = filearray_get(filetable, fd);
	if (f == NULL) return EBADF;

	// Remove fd
	filearray_remove(filetable, fd);

	// Delete if refcount is 0 after decrement
	if (--f->f_refcount == 0) {
		lock_destroy(f->f_lock);
		kfree(f);
	}

	lock_release(filetable_lock);
}