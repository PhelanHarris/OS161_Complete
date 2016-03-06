#include <proctable.h>
#include <proc.h>

void proctable_cleanup (struct proctable *pt);
int proctable_preallocate(struct proctable *pt, unsigned num);
int proctable_setsize(struct proctable *pt, unsigned num);


struct proctable *
proctable_create () {
	pt->size = pt->num = pt->max = 0;
	pt->v = NULL;
}

int
proctable_add (struct proctable *pt, proc* p) {
	proctable_setsize(pt, pt->num+1);

	int i;
	for (i = 2; i < pt->num; i++) {
		if (pt->v[i] == NULL) {
			pt->v[i] = p;
			pt-num++;
			return i;
		}
	}
}

void
proctable_remove (struct proctable *pt, int pid) {
	pt->v[pid] = NULL;
	pt->num--;
	proctable_cleanup(pt);
}

void 
proctable_cleanup (struct proctable *pt)
{
	int i = pt->num - 1;
	while (pt->v[i] = NULL){
		
	}
}

int
proctable_preallocate(struct proctable *pt, unsigned num)
{
	void **newptr;
	unsigned newmax;

	if (num > pt->max) {
		/* Don't touch A until the allocation succeeds. */
		newmax = pt->max;
		while (num > newmax) {
			newmax = newmax ? newmax*2 : 4;
		}

		/*
		 * We don't have krealloc, and it wouldn't be
		 * worthwhile to implement just for this. So just
		 * allocate a new block and copy. (Exercise: what
		 * about this and/or kmalloc makes it not worthwhile?)
		 */

		newptr = kmalloc(newmax*sizeof(*pt->v));
		if (newptr == NULL) {
			return ENOMEM;
		}
		memcpy(newptr, pt->v, pt->num*sizeof(*pt->v));
		kfree(pt->v);
		pt->v = newptr;
		pt->max = newmax;
	}
	return 0;
}

int
proctable_setsize(struct proctable *pt, unsigned num)
{
	int result;

	result = proctable_preallocate(pt, num);
	if (result) {
		return result;
	}
	pt->num = num;

	return 0;
}