/*
 * Filetable Implementation
 */

#define FILETABLEINLINE

#include <filetable.h>
#include <vfs.h>
#include <vnode.h>
#include <limits.h>
#include <types.h>
#include <spinlock.h>
#include <kern/errno.h>
#include <kern/fcntl.h>

int getLowestIndex(struct filetable *ft);

int
getLowestIndex(struct filetable *ft)
{
	int i;
	for (i = 0; i < OPEN_MAX; ++i)
		if (ft->ft_arr[i] == NULL) 
			return i;
	return -1;
}

int
filetable_init(struct filetable *ft)
{	
	struct vnode *in_vn, *out_vn, *err_vn;
	char *in_str = NULL, *out_str = NULL, *err_str = NULL;
	int result;
	unsigned i;

	// Init STDIN
	in_str = kstrdup("con:");
	result = vfs_open(in_str, O_RDONLY, 0, &in_vn);
	if (result) {
		kfree(in_str);
		vfs_close(in_vn);
		return result;
	}
	
	// Init STDOUT
	out_str = kstrdup("con:");
	result = vfs_open(out_str, O_WRONLY, 0, &out_vn);
	if (result) {
		kfree(in_str);
		kfree(out_str);
		vfs_close(in_vn);
		vfs_close(out_vn);
		return result;
	}

	// Init STDERR
	err_str = kstrdup("con:");
	result = vfs_open(err_str, O_WRONLY, 0, &err_vn);
	if (result) {
		kfree(in_str);
		kfree(out_str);
		kfree(err_str);
		vfs_close(in_vn);
		vfs_close(out_vn);
		vfs_close(err_vn);
		return result;
	}

	// Init structs
	ft->ft_arr = (struct file **) kmalloc(sizeof(struct file *)*OPEN_MAX);
	for (i = 0; i < OPEN_MAX; i++){
		ft->ft_arr[i] = NULL;
	}

	ft->ft_lock = lock_create("filetablelock");
	ft->ft_size = 0;

	// Add standard streams
	unsigned fd_ret = 0;
	result = filetable_add(ft, in_vn, O_RDONLY, &fd_ret);
	if (result) {
		filetable_destroy(ft);
		return result;
	}

	result = filetable_add(ft, out_vn, O_WRONLY, &fd_ret);
	if (result) {
		filetable_destroy(ft);
		return result;
	}

	result = filetable_add(ft, err_vn, O_WRONLY, &fd_ret);
	if (result) {
		filetable_destroy(ft);
		return result;
	}

	return 0;
}

void
filetable_destroy(struct filetable *ft)
{
	unsigned i;
	for (i = 0; i < OPEN_MAX; ++i) {
		if (ft->ft_arr[i] != NULL) {
			vfs_close(ft->ft_arr[i]->f_vn);
			filetable_remove(ft, i);
		}
	}

	lock_destroy(ft->ft_lock);
	kfree(ft->ft_arr);
}

int
filetable_get(struct filetable *ft, unsigned fd, struct file **f_ret)
{
	if (fd >= OPEN_MAX) return EBADF;
	*f_ret = ft->ft_arr[fd];

	return *f_ret == NULL ? EBADF : 0;
}

/*
 * Creates a new file object based on the provided vnode. On success, returns 0
 * and returns the file descriptor in fd_ret. Otherwise, may return the error
 * code and fd_ret will be null.
 */
int
filetable_add(struct filetable *ft, struct vnode *vn, int flags, unsigned *fd_ret)
{
	struct file *f;

	// Make atomic
	lock_acquire(ft->ft_lock);

	// Reject if too many files open
	if (ft->ft_size == OPEN_MAX) {
		lock_release(ft->ft_lock);
		return EMFILE;
	}

	int result = getLowestIndex(ft);
	if (result == -1){
		KASSERT(result != -1);
		lock_release(ft->ft_lock);
		return EMFILE;
	}
	*fd_ret = (unsigned) result;

	
	// Create new file object
	f = (struct file *) kmalloc(sizeof(struct file));
	f->f_vn = vn;
	f->f_flags = flags;
	f->f_cursor = 0;
	f->f_refcount = 1;
	f->f_lock = lock_create("filelock"); // name is not important

	// Add file object to filetable

	ft->ft_arr[*fd_ret] = f;

	ft->ft_size++;
	lock_release(ft->ft_lock);
	return 0;
}

/*
 * Clones a file descriptor into a new one. Returns 0 on success, or the error
 * code. The new file descriptor must point to an empty spot.
 */
int
filetable_clone(struct filetable *ft, unsigned fd_old, unsigned fd_new)
{
	struct file *f = NULL;

	if (fd_old > OPEN_MAX || fd_new > OPEN_MAX) return EBADF;
	KASSERT(ft->ft_arr[fd_new] == NULL);

	// Make atomic
	lock_acquire(ft->ft_lock);

	// Reject if too many files open
	if (ft->ft_size == OPEN_MAX) {
		lock_release(ft->ft_lock);
		return EMFILE;
	}

	// Get file object
	f = ft->ft_arr[fd_old];
	if (f == NULL) {
		lock_release(ft->ft_lock);
		return EBADF;
	}
	
	// Add new fd
	f->f_refcount++;
	ft->ft_arr[fd_new] = f;

	ft->ft_size++;
	lock_release(ft->ft_lock);
	return 0;
}

/*
 * Removes a file descriptor and the file object if no other fd points to it.
 */
int
filetable_remove(struct filetable *ft, unsigned fd) {
	struct file *f = NULL;

	if (fd >= OPEN_MAX) return EBADF;

	// Make atomic
	lock_acquire(ft->ft_lock);

	// Check if fd is valid
	f = ft->ft_arr[fd];
	if (f == NULL) {
		lock_release(ft->ft_lock);
		return EBADF;
	}

	// Remove fd
	ft->ft_arr[fd] = NULL;
	f->f_refcount--;
	ft->ft_size--;


	// Close vnode and delete once refcount is 0
	if (f->f_refcount == 0) {
		// Close at vfs level
		lock_acquire(f->f_lock);
		vfs_close(f->f_vn);
		lock_release(f->f_lock);

		// Cleanup
		lock_destroy(f->f_lock);
		kfree(f);
	}

	lock_release(ft->ft_lock);
	return 0;
}