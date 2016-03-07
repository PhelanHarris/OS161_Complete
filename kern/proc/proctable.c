#include <proctable.h>
#include <proc.h>
#include <types.h>
#include <kern/errno.h>

// Global process table
struct proctable *proctable;

// Private function prototypes
int proctable_preallocate(unsigned num);
int proctable_setsize(unsigned num);

/**
 * Initializes the process table.
 */
int
proctable_init()
{
	proctable = kmalloc(sizeof(*proctable));
	if (proctable == NULL) {`
		return ENOMEM;
	}

	proctable->size = proctable->num = 0;
	proctable->v = NULL;

	return 0;
}

/**
 * Adds a process to the process table. Returns it's new pid through the second
 * parameter. Returns an error code.
 */
int
proctable_add(struct proc *p, pid_t *ret_pid)
{
	int i;
	int result = 0;

	i = proctable->num;
	proctable->num++;
		
	// Check if table needs to be expanded
	if (proctable->num > proctable->size) {
		result = proctable_setsize(proctable->num);
		if (result) {
			return result;
		}

		proctable->v[i] = p;
		*ret_pid = i;
		return 0;
	}

	// Fill in the gaps
	for (i = 0; i < proctable->size; i++) {
		if (proctable->v[i] == NULL) {
			proctable->v[i] = p;
			*ret_pid = i;
			return 0;
		}
	}

	panic("Process table smaller than its recorded size.")
}

/**
 * Removes a process from the process table. Returns an error code.
 */
int
proctable_remove(pid_t pid)
{
	proctable->v[pid] = NULL;
	proctable->num--;

	int new_size = proctable->size;
	int i = proctable->num - 1;
	while (proctable->v[i] == NULL){
		i--;
		new_size--;
	}

	if (new_size != proctable->size) {
		return proctable_setsize(new_size);
	} else {
		return 0;
	}
}

/**
 * Sets the size of the process table to at least the size specified.
 */
int
proctable_setsize(unsigned size)
{
	int result;

	result = proctable_preallocate(size);
	if (result) {
		return result;
	}

	return 0;
}

/**
 * Allocates a new process table of at least the size specified.
 */
int
proctable_preallocate(unsigned size)
{
	void **newptr;
	unsigned new_size;

	if (size > proctable->size) {
		// Get new size
		new_size = proctable->size;
		while (size > new_size) {
			new_size = new_size ? new_size*2 : 4;
		}

		// Allocate new table
		newptr = kmalloc(new_size*sizeof(*proctable->v));
		if (newptr == NULL) {
			return ENOMEM;
		}

		// Copy and free old one
		memcpy(newptr, proctable->v, proctable->num*sizeof(*proctable->v));
		kfree(proctable->v);
		proctable->v = newptr;
		proctable->size = new_size;
	}
	return 0;
}
