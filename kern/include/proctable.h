#ifndef _PROCARRAY_H_
#define _PROCARRAY_H_

#include <proc.h>

struct proctable_entry {
	proc *pte_p;
	int pte_exitcode;
	struct cv *pte_cv;
	struct lock *pte_lock;
};

struct proctable {
	proctable_entry **pt_v;
	unsigned pt_num, pt_size;
};

extern struct proctable *proctable;

struct proctable *proctable_create(void);
pid_t proctable_add(struct proctable *pt, proc* p);
struct proctable_entry *proctable_get(pid_t pid)
int proctable_remove(struct proctable *pt, pid_t pid);

#endif /* _PROCARRAY_H_ */