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
	int result;
	struct file *f;

	// Make atomic
	lock_acquire(filetable_lock);

	// TODO: check if not at max number of files (return EMFILE/ENFILE)
	
	// Create new file object & add to filetable
	f = kmalloc(sizeof(struct file));
	result = filearray_add(filetable, f, fd_ret);
	if (result) {
		kfree(k);
		return result;
	}

	// Populate
	f->f_vn = vn;
	f->f_mode = 0;
	f->f_cursor = 0;
	f->f_refcount = 1;
	f->f_lock = lock_create("filelock"); // name is not important

	lock_release(filetable_lock);
	return result;
}

/*
 * Clones a file descriptor into a new one. Returns 0 on success, or the error
 * code. The new file descriptor must point to an empty spot.
 */
int
filetable_clone(unsigned fd_old, unsigned fd_new)
{
	int result;
	struct file *f = NULL;

	KASSERT(filearray_get(filetable, fd_new) == NULL);

	// Make atomic
	lock_acquire(filetable_lock);

	f = filearray_get(filetable, fd);
	if (f == NULL) return EBADF;

	// TODO: if max number of files return EMFILE (proc) / ENFILE (sys-wide)
	
	f->refcount++;
	result = filearray_add(filetable, f, fd_new);
	if (result) {
		f->refcount--;
		lock_release(filetable_lock);
		return result;
	}

	lock_release(filetable_lock);
	return 0;
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