#include <proctable.h>
#include <proc.h>

void proctable_cleanup (struct proctable *pt);

struct proctable *
proctable_create () {
	pt->num = pt->max = 0;
}

int
proctable_add (struct proctable *pt, proc* p) {
	int i;
	for (i = 2; i < pt->num; i++) {
		if (pt->v[i] == NULL) {
			pt->v[i] = p;
			return i;
		}
	}
}

void
proctable_remove (struct proctable *pt, int pid) {
	pt->v[pid] = NULL;
	proctable_cleanup(pt);
}

void 
proctable_cleanup (struct proctable *pt)
{
	int i = pt->num - 1;
	while (pt->v[i] = NULL){
		
	}
}