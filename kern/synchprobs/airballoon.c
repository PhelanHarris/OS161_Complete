/*
 * Driver code for airballoon problem
 */
#include <types.h>
#include <lib.h>
#include <thread.h>
#include <test.h>
#include <synch.h>

#define NROPES 16
#define CUT -1 // special value to put in the ropes array if it is cut
static int ropes_left = NROPES;

// Data structures for rope mappings

static int ropes[NROPES];	// the index is the hook (and rope number),
							// and the value is the attached stake,
							// or -1 if there is the rope is cut


// Synchronization primitives 

static struct lock *rope_locks[NROPES];	// Locks for each of the ropes indices
static struct lock *ropes_left_lock;	// Lock to decrement ropes_left
static struct semaphore *done_sem;		// semaphore to tell when all threads have exit
static struct semaphore *freed_sem;		// semaphore to tell when all all ropes have been cut for the balloon thread

/*
 * Describe your design and any invariants or locking protocols 
 * that must be maintained. Explain the exit conditions. How
 * do all threads know when they are done?  
 */

static
void
dandelion(void *p, unsigned long arg)
{
	(void)p;
	(void)arg;
	
	kprintf("Dandelion thread starting\n");

	while (ropes_left > 0){	// exits when ropes_left == 0 (from either dandelion or marigold cutting the last rope)

		int hook = random() % NROPES; // the hook is equivalent to the the rope index
		if (ropes[hook] != CUT) {							// check that the rope has not been cut yet
			lock_acquire(rope_locks[hook]);					// acquire that rope's lock
			if (ropes[hook] != CUT){						// check again that the rope has not been cut since the first check
				ropes[hook] = CUT;								
				kprintf("Dandelion severed rope %d\n", hook);	
				lock_acquire(ropes_left_lock);				// acquire the ropes_left lock
				ropes_left--;								// to ensure ropes_left-- is atomic
				lock_release(ropes_left_lock);				// then release it
				lock_release(rope_locks[hook]);		// release the rope's lock and then yield the thread
				thread_yield();
			}
			else {
				lock_release(rope_locks[hook]);		// if the rope was cut between checking and
			}										// acquiring the lock, just release the lock
		}
	}

	V(freed_sem); // to tell the balloon thread that this thread is done
	kprintf("Dandelion thread done\n");
	V(done_sem); // to tell the main thread that this thread is done
}

static
void
marigold(void *p, unsigned long arg)
{
	(void)p;
	(void)arg;
	
	kprintf("Marigold thread starting\n");

	while (ropes_left > 0){ // exits when ropes_left == 0

		int stake = random() % NROPES;	// generates a random stake number
		int ropeIndex;
		for (ropeIndex = 0; ropeIndex < NROPES; ropeIndex++){	// iterates through the ropes to see 
			if (ropes[ropeIndex] == stake){						// if that stake has a rope attached to it.
				lock_acquire(rope_locks[ropeIndex]);			// If it does, acquire that rope's lock.
				if (ropes[ropeIndex] == stake){					// Double check that the rope hasn't been cut or moved
					ropes[ropeIndex] = CUT;						
					kprintf("Marigold severed rope %d from stake %d\n", ropeIndex, stake);
					lock_acquire(ropes_left_lock);			// acquire the ropes_left lock
					ropes_left--;							// to ensure ropes_left -- is atomic
					lock_release(ropes_left_lock);			// then release it
					lock_release(rope_locks[ropeIndex]);		// release the rope's lock and yield the thread
					thread_yield();
				}
				else{
					lock_release(rope_locks[ropeIndex]);	// if the rope was cut or moved between first check
				}											// and acquiring the lock, just release the lock
				break;		// break out of the loop because we found a rope
			}
		}
	}
	V(freed_sem); 	// to tell the balloon thread that this thread is done
	kprintf("Marigold thread done\n");
	V(done_sem); 	// to tell the main thread that this thread is done
}

static
void
flowerkiller(void *p, unsigned long arg)
{
	(void)p;
	(void)arg;
	
	kprintf("Lord FlowerKiller thread starting\n");

	while (ropes_left > 0){ // exits when ropes_left == 0

		int oldStake = random() % NROPES;	// generates a random stake number
		int i;
		for (i = 0; i < NROPES; i++){		// iterates through the ropes to see 
			if (ropes[i] == oldStake){		// if that stake has a rope attached to it.
				lock_acquire(rope_locks[i]);	// if it does, acquire that rope's lock
				if (ropes[i] == oldStake){		// and double check that the rope hasn't been cut or moved
					int newStake = random() % (NROPES - 1);	// generate a new random stake
					if (newStake >= oldStake) {				// make sure it isn't the same stake
						newStake++;
					}
					kprintf("Lord FlowerKiller switched rope %d from stake %d to stake %d\n", i, oldStake, newStake);
					ropes[i] = newStake;			// move the rope to the new stake
					lock_release(rope_locks[i]);	// release the rope's lock and yield the thread
					thread_yield();
				}
				else{
					lock_release(rope_locks[i]);	// if the rope was cut between the first check
				}									// and acquiring the lock, just realease the lock
				break;		// break out of the loop because we found a rope
			}
		}
	}

	V(freed_sem);	// to tell the balloon thread that this thread is done
	kprintf("Lord FlowerKiller thread done\n");
	V(done_sem);	// to tell the main thread that this thread is done
}

static
void
balloon(void *p, unsigned long arg)
{
	(void)p;
	(void)arg;
	
	kprintf("Balloon thread starting\n");
	
	int i;
	for (i = 0; i < 3; i++){	// semaphore used to tell when the dandelion, marigold, and
		P(freed_sem);			// flower killer threads have finished and called V() on freed_sem
	}

	kprintf("Balloon freed and Prince Dandelion escapes!\n");
	kprintf("Balloon thread done\n");
	V(done_sem);	// to tell the main thread that this thread is done
}


// Change this function as necessary
int
airballoon(int nargs, char **args)
{

	int err = 0;

	(void)nargs;
	(void)args;
	(void)ropes_left;
	
	ropes_left = NROPES;	// reset every time the test is run

	int i;
	for (i = 0; i < NROPES; i++){	
		ropes[i] = i;								// set each rope to initially be at the same hook and stake
		rope_locks[i] = lock_create("lock name");	// create each rope's lock
	}
	
	ropes_left_lock = lock_create("lock name");		// create the ropes_left lock and the semaphores
	done_sem = sem_create("semaphore name", 0);
	freed_sem = sem_create("semaphore name", 0);

	err = thread_fork("Marigold Thread",
			  NULL, marigold, NULL, 0);
	if(err)
		goto panic;
	
	err = thread_fork("Dandelion Thread",
			  NULL, dandelion, NULL, 0);
	if(err)
		goto panic;
	
	err = thread_fork("Lord FlowerKiller Thread",
			  NULL, flowerkiller, NULL, 0);
	if(err)
		goto panic;

	err = thread_fork("Air Balloon",
			  NULL, balloon, NULL, 0);
	if(err)
		goto panic;

	goto done;
panic:
	panic("airballoon: thread_fork failed: %s)\n",
	      strerror(err));
	
done:
	for (i = 0; i < 4; i++){			
		P(done_sem);					// semaphore to wait until the other 4 threads have finished
	}

	lock_destroy(ropes_left_lock);		// clean up when everyone is done
	sem_destroy(done_sem);
	sem_destroy(freed_sem);

	for (i = 0; i < NROPES; i++){
		lock_destroy(rope_locks[i]);
	}
	kprintf("Main thread done\n");
	return 0;
}
