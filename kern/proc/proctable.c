#include <proctable.h>
#include <proc.h>
#include <types.h>

void proctable_cleanup (struct proctable *pt);
int proctable_preallocate(struct proctable *pt, unsigned num);
int proctable_setsize(struct proctable *pt, unsigned num);


struct proctable *
proctable_create () {
	struct proctable *pt;

	pt = kmalloc(sizeof(*pt));
	if (pt != NULL) {
		pt->size = pt->num = 0;
		pt->v = NULL;
	}
	return pt;
}

int
proctable_add (struct proctable *pt, proc* p, pid_t *pid) {
	pt->num++;
	
	int ret = 0;
	if (pt->num > pt->size){
		ret = proctable_setsize(pt, pt->num);
		if (ret)
			return ret;
		*pid = pt->num - 1;
		pt->v[*pid] = p;
		return 0;
	}

	int i;
	for (i = 2; i < pt->size; i++) {
		if (pt->v[i] == NULL) {
			pt->v[i] = p;
			*pid = i;
			return 0;
		}
	}

}

int
proctable_remove (struct proctable *pt, int pid) {
	pt->v[pid] = NULL;
	pt->num--;
	return proctable_cleanup(pt);
}

int 
proctable_cleanup (struct proctable *pt)
{
	int newSize = pt->size;
	int i = pt->num - 1;
	while (pt->v[i] == NULL){
		i--;
		newSize--;
	}

	if (newSize != pt->size) {
		return proctable_setsize(pt, newSize);
	} else {
		return 0;
	}
}

int
proctable_preallocate(struct proctable *pt, unsigned num)
{
	void **newptr;
	unsigned newSize;

	if (num > pt->size) {
		newSize = pt->size;
		while (num > newSize) {
			newSize = newSize ? newSize*2 : 4;
		}

		newptr = kmalloc(newSize*sizeof(*pt->v));
		if (newptr == NULL) {
			return ENOMEM;
		}
		memcpy(newptr, pt->v, pt->num*sizeof(*pt->v));
		kfree(pt->v);
		pt->v = newptr;
		pt->size = newSize;
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