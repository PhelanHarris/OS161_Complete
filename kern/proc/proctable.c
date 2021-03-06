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
int proctable_setsize(unsigned num);

/**
 * Initializes the process table.
 */
void
proctable_init(void)
{
	proctable = (struct proctable *) kmalloc(sizeof(*proctable));
	if (proctable == NULL) {
		panic("proctable_init for global process table failed\n");		
	}

	proctable->pt_num = 0;
	proctable->pt_size = PID_MAX;
	proctable->pt_v = (struct proctable_entry **) kmalloc(
		sizeof(*(proctable->pt_v)) * PID_MAX);

	spinlock_init(&proctable_lock);
}

/**
 * Adds a process to the process table. Returns its new pid through the second
 * parameter. Returns an error code.
 */
int
proctable_add(struct proc *p, pid_t *ret_pid)
{
	unsigned i;
	struct proctable_entry *pte;

	spinlock_acquire(&proctable_lock);

	// Check if over limit
	if ((proctable->pt_num + 1) == PID_MAX) {
		spinlock_release(&proctable_lock);
		return ENPROC;
	}

	// Create entry
	pte = proctable_create_entry(p);
	if (pte == NULL) {
		spinlock_release(&proctable_lock);
		return ENOMEM;
	}

	// Find a spot in the table
	for (i = 0; i < PID_MAX; i++) {
		if (proctable->pt_v[i] == NULL) {
			proctable->pt_v[i] = pte;
			proctable->pt_num++;
			*ret_pid = i;
			spinlock_release(&proctable_lock);
			return 0;
		}
	}

	panic("No vacant spot found in the process table, with size %d, and %d entries.", proctable->pt_size, proctable->pt_num);
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
	pte->pte_refcount = 1;
	pte->pte_exitcode = -1;
	pte->pte_running = true;
	
	// Create condition variable
	pte->pte_cv = cv_create(pte->pte_p->p_name);
	if (pte->pte_cv == NULL) {
		kfree(pte);
		return NULL;
	}

	// Create lock
	pte->pte_lock = lock_create(pte->pte_p->p_name);
	if (pte->pte_lock == NULL) {
		cv_destroy(pte->pte_cv);
		kfree(pte);
		return NULL;
	}
	return pte;
}

/**
 * Gets the proctable_entry with the specified pid. Returns NULL if no such
 * entry was found.
 */
struct proctable_entry *
proctable_get(pid_t pid)
{
	if (pid > PID_MAX || pid > (int)proctable->pt_size) {
		return NULL;
	}

	return proctable->pt_v[pid];
}

/**
 * Removes a process from the process table. Returns true if the proctabe_entry
 * was actually removed (refcount was 0), otherwise false.
 */
bool
proctable_remove(pid_t pid)
{	
	// Get PTE
	struct proctable_entry *pte = proctable->pt_v[pid];
	KASSERT(pte != NULL);

	// Make sure atomic
	if (!lock_do_i_hold(pte->pte_lock))
		lock_acquire(pte->pte_lock);

	// Decrement refcount
	pte->pte_refcount--;

	// Return if refs still exist
	if (pte->pte_refcount != 0)
		return false;

	// Cleanup
	lock_release(pte->pte_lock);
	lock_destroy(pte->pte_lock);
	cv_destroy(pte->pte_cv);
	kfree(pte);

	// Reclaim pid
	proctable->pt_v[pid] = NULL;

	// Determine new size of array
	/*spinlock_acquire(&proctable_lock);

	proctable->pt_num--;
	unsigned new_size = proctable->pt_size;
	int i = proctable->pt_num - 1;
	while (i > 0 && proctable->pt_v[i] == NULL){
		i--;
		new_size--;
	}

	// Trim the end of the array if needed (must not fail)
	if (new_size != proctable->pt_size) {
		KASSERT(proctable_setsize(new_size) == 0);
	}

	spinlock_release(&proctable_lock);*/

	return true;
}

/**
 * Sets the size of the process table to at least the size specified.
 */
int
proctable_setsize(unsigned size)
{
	struct proctable_entry **newptr;
	unsigned new_size;

	KASSERT(spinlock_do_i_hold(&proctable_lock));

	if (size > proctable->pt_size) {
		// Get new size
		new_size = proctable->pt_size;
		while (size > new_size) {
			new_size = new_size ? new_size*2 : PID_MAX/2;
		}

		// Allocate new table
		newptr = (struct proctable_entry **) kmalloc(new_size*sizeof(*proctable->pt_v));
		if (newptr == NULL) {
			return ENOMEM;
		}

		// Copy and free old one
		if (proctable->pt_v) {
			memcpy(newptr, proctable->pt_v, proctable->pt_num*sizeof(*proctable->pt_v));
			kfree(proctable->pt_v);
		}
		proctable->pt_v = newptr;
		proctable->pt_size = new_size;
	}
	return 0;
}
