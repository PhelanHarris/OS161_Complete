#include <types.h>
#include <proc.h>
#include <synch.h>
#include <limits.h>
#include <kern/errno.h>
#include <proctable.h>

// Global process table
struct proctable *proctable;

// Private function prototypes
struct proctable_entry *proctable_create_entry(struct proc *p);
int proctable_preallocate(unsigned num);
int proctable_setsize(unsigned num);

/**
 * Initializes the process table.
 */
/*int
proctable_init()
{
	proctable = (struct proctable *) kmalloc(sizeof(*proctable));
	if (proctable == NULL) {
		return ENOMEM;
	}

	proctable->pt_size = proctable->pt_num = 0;
	proctable->pt_v = NULL;

	return 0;
}*/

/**
 * Adds a process to the process table. Returns its new pid through the second
 * parameter. Returns an error code.
 */
int
proctable_add(struct proc *p, pid_t *ret_pid)
{
	unsigned i;
	int result = 0;
	struct proctable_entry *pte;

	// Check if over limit
	if (proctable->pt_num == PID_MAX) {
		return ENPROC;
	}

	// Create entry
	pte = proctable_create_entry(p);
	if (pte == NULL) {
		return ENOMEM;
	}

	// Find a spot in the table
	i = proctable->pt_num;
	proctable->pt_num++;
		
	// Check if table needs to be expanded
	if (proctable->pt_num > proctable->pt_size) {
		result = proctable_setsize(proctable->pt_num);
		if (result) {
			return result;
		}

		proctable->pt_v[i] = pte;
		*ret_pid = i;
		return 0;
	}

	// Find a gap in the table
	for (i = 0; i < proctable->pt_size; i++) {
		if (proctable_get(i) == NULL) {
			proctable->pt_v[i] = pte;
			*ret_pid = i;
			return 0;
		}
	}

	panic("Process table smaller than its recorded size.");
}

/**
 * Creates and initializes a proctable_entry struct.
 */
struct proctable_entry *
proctable_create_entry(struct proc *p)
{
	struct proctable_entry *pte;

	pte = (struct proctable_entry *) kmalloc(sizeof(*pte));
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
	if (pid < PID_MIN || pid > PID_MAX || pid > (int)proctable->pt_size) {
		return NULL;
	}

	return proctable->pt_v[pid];
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
	lock_destroy(pte->pte_lock);
	kfree(pte);

	proctable->pt_v[pid] = NULL;
	proctable->pt_num--;

	// Trim end of the array if needed
	unsigned new_size = proctable->pt_size;
	int i = proctable->pt_num - 1;
	while (i > 0 && proctable_get(i) == NULL){
		i--;
		new_size--;
	}

	if (new_size != proctable->pt_size) {
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

	if (size > proctable->pt_size) {
		// Get new size
		new_size = proctable->pt_size;
		while (size > new_size) {
			new_size = new_size ? new_size*2 : 4;
		}

		// Allocate new table
		newptr = kmalloc(new_size*sizeof(*proctable->pt_v));
		if (newptr == NULL) {
			return ENOMEM;
		}

		// Copy and free old one
		memcpy(newptr, proctable->pt_v, proctable->pt_num*sizeof(*proctable->pt_v));
		kfree(proctable->pt_v);
		proctable->pt_v = (struct proctable_entry **)newptr;
		proctable->pt_size = new_size;
	}
	return 0;
}
