#include <proctable.h>
#include <proc.h>
#include <array.h>

void proctable_cleanup (struct array *pt);

void
proctable_init (struct array *pt) 
{

}

pid_t
proctable_add (struct array *pt, proc* p) 
{
	int i;
	for (i = 2; i < pt->num; i++) {
		if (pt->v[i] == NULL) {
			pt->v[i] = p;
			return i;
		}
	}
}

void
proctable_remove (struct array *pt, pid_t pid) 
{
	pt->v[pid] = NULL;
	proctable_cleanup(pt);
}

void 
proctable_cleanup (struct array *pt)
{
	int i = pt->num - 1;
	while (pt->v[i] = NULL){
		
	}
}