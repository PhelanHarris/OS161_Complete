/*
 * Filetable Implementation
 */

#define FILETABLEINLINE

#include <filetable.h>
#include <vnode.h>
#include <spinlock.h>
#include <kern/errno.h>
#include <vfs.h>

unsigned
getLowestIndex(struct filetable *ft)
{
	unsigned i;
	for (i = 0; i < MAX_OPEN; ++i)
		if (ft->ft_arr[i] == NULL) 
			return i;
	return -1;
}

int
filetable_init(struct filetable *ft)
{	
	struct vnode *in_vn, *out_vn, *err_vn;
	char *in_str = NULL, *out_str = NULL, *err_srt = NULL;
	int result;

	// Init STDIN
	in_str = kstrdup("con:");
	result = vfs_open(in_str, O_RDONLY, 0, in_vn);
	if (result) {
		kfree(in_str);
		vfs_close(in_vn);
		return result;
	}
	
	// Init STDOUT
	out_str = kstrdup("con:");
	result = vfs_open(out_str, O_WRONLY, 0, out_vn);
	if (result) {
		kfree(in_str);
		kfree(out_str);
		vfs_close(in_vn);
		vfs_close(out_vn);
		return result;
	}

	// Init STDERR
	err_str = kstrdup("con:");
	result = vfs_open(err_str, O_WRONLY, 0, err_vn);
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
	ft->ft_arr = kmalloc(sizeof(struct file *)*MAX_OPEN);
	ft->ft_lock = lock_create("filetablelock");

	// Add standard streams
	result = filetable_add(ft, in_vn, NULL);
	if (result) {
		filetable_destory(ft);
		return result;
	}

	result = filetable_add(ft, out_vn, NULL);
	if (result) {
		filetable_destory(ft);
		return result;
	}

	result = filetable_add(ft, err_vn, NULL);
	if (result) {
		filetable_destory(ft);
		return result;
	}

	return 0;
}

void
filetable_destory(struct filetable *ft)
{
	unsigned i;
	for (i = 0; i < MAX_OPEN; ++i) {
		if (ft->ft_arr[i] != NULL) {
			vps_close(ft_arr[i]->f_vn);
			filetable_remove(ft, i);
		}
	}

	lock_destroy(ft->ft_lock);
	kfree(ft->ft_arr);
}

int
filetable_get(struct filetable *ft, unsigned fd, struct file **f_ret)
{
	if (fd > MAX_OPEN) return EBADF;
	*f_ret = ft->ft_arr[fd];

	return f_ret == NULL ? EBADF : 0;
}

/*
 * Creates a new file object based on the provided vnode. On success, returns 0
 * and returns the file descriptor in fd_ret. Otherwise, may return the error
 * code and fd_ret will be null.
 */
int
filetable_add(struct filetable *ft, vnode *vn, unsigned *fd_ret)
{
	int result;
	struct file *f;

	// Make atomic
	lock_acquire(ft->ft_lock);

	// Reject if too many files open
	if (ft->ft_size == MAX_OPEN) {
		lock_release(ft->ft_lock);
		return EMFILE;
	}
	
	// Create new file object
	f = kmalloc(sizeof(struct file));
	f->f_vn = vn;
	f->f_mode = 0;
	f->f_cursor = 0;
	f->f_refcount = 1;
	f->f_lock = lock_create("filelock"); // name is not important

	// Add file object to filetable
	*fd_ret = getLowestIndex(ft);
	ft->ft_arr[*fd_ret] = f;

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
	int result;
	struct file *f = NULL;

	if (fd_old > MAX_OPEN || fd_new > MAX_OPEN) return EBADF;
	KASSERT(ft->ft_arr[fd_new] == NULL);

	// Make atomic
	lock_acquire(ft->ft_lock);

	// Reject if too many files open
	if (ft->ft_size == MAX_OPEN) {
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
	f->refcount++;
	ft->ft_arr[fd_new] = f;

	lock_release(ft->ft_lock);
	return 0;
}

/*
 * Removes a file descriptor and the file object if no other fd points to it.
 */
int
filetable_remove(struct filetable *ft, unsigned fd) {
	struct file *f = NULL;

	if (fd > MAX_OPEN) return EBADF;

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

	// Delete if refcount is 0 after decrement
	if (f->f_refcount == 0) {
		lock_destroy(f->f_lock);
		kfree(f);
	}

	lock_release(ft->ft_lock);
	return 0;
}