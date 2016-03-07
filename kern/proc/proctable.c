#include <proctable.h>
#include <proc.h>
#include <types.h>
#include <synch.h>
#include <limits.h>
#include <kern/errno.h>

// Global process table
struct proctable *proctable;

// Private function prototypes
struct proctable_entry *proctable_create_entry(struct proc *p);
int proctable_preallocate(unsigned num);
int proctable_setsize(unsigned num);

/**
 * Initializes the process table.
 */
int
proctable_init()
{
	proctable = (proctable *) kmalloc(sizeof(*proctable));
	if (proctable == NULL) {`
		return ENOMEM;
	}

	proctable->size = proctable->num = 0;
	proctable->v = NULL;

	return 0;
}

/**
 * Adds a process to the process table. Returns its new pid through the second
 * parameter. Returns an error code.
 */
int
proctable_add(struct proc *p, pid_t *ret_pid)
{
	int i;
	int result = 0;
	struct proctable_entry *pte;

	// Check if over limit
	if (proctable->num == PID_MAX) {
		return ENPROC;
	}

	// Create entry
	pte = proctable_create_entry(p);
	if (pte == NULL) {
		return ENOMEM;
	}

	// Find a spot in the table
	i = proctable->num;
	proctable->num++;
		
	// Check if table needs to be expanded
	if (proctable->num > proctable->size) {
		result = proctable_setsize(proctable->num);
		if (result) {
			return result;
		}

		*(proctable->v)[i] = pte;
		*ret_pid = i;
		return 0;
	}

	// Find a gap in the table
	for (i = 0; i < proctable->size; i++) {
		if (proctable_get(i) == NULL) {
			*(proctable->v)[i] = pte;
			*ret_pid = i;
			return 0;
		}
	}

	panic("Process table smaller than its recorded size.")
}

/**
 * Creates and initializes a proctable_entry struct.
 */
struct proctable_entry *
proctable_create_entry(struct proc *p)
{
	struct proctable_entry *pte;

	pte = (proctable_entry *) kmalloc(sizeof(*pte));
	if (pte == NULL) {
		return NULL;
	}

	// Set values
	pte->pte_p = p;
	pte->pte_exitcode = -1;
	pte->pte_cv = cv_create(pte->pte_p->p_name);
	pte->pte_lock = lock_create(pte->pte_p->p_name);

	return pte;
}

/**
 * Gets the proctable_entry with the specified pid. Returns NULL if no such
 * entry was found.
 */
struct proctable_entry *
proctable_get(pid_t pid)
{
	if (pid < PID_MIN || pid > PID_MAX || pid > proctable->size) {
		return NULL;
	}

	return *(proctable->v)[pid];
}

/**
 * Removes a process from the process table. Returns an error code.
 */
int
proctable_remove(pid_t pid)
{
	// Assume the process has been properly destoryed
	struct proctable_entry *pte = proctable_get(pid);
	if (pte == NULL) {
		return 0;
	}

	cv_destroy(pte->pte_cv);
	lock_destroy(pte->pte_lock)
	kfree(pte);

	*(proctable->v)[pid] = NULL;
	proctable->num--;

	// Trim end of the array if needed
	int new_size = proctable->size;
	int i = proctable->num - 1;
	while (i > 0 && proctable_get(i) == NULL){
		i--;
		new_size--;
	}

	if (new_size != proctable->size) {
		return proctable_setsize(new_size);
	}

	return 0;
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
