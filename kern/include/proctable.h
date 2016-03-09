#ifndef _PROCARRAY_H_
#define _PROCARRAY_H_

#include <proc.h>

struct proctable_entry {
	struct proc *pte_p;
	int pte_exitcode;
	struct cv *pte_cv;
	struct lock *pte_lock;
};

struct proctable {
	struct proctable_entry **pt_v;
	unsigned pt_num, pt_size;
};

extern struct proctable *proctable;

int proctable_init();
int proctable_add(struct proc *p, pid_t *ret_pid);
struct proctable_entry *proctable_get(pid_t pid);
int proctable_remove(pid_t pid);

#endif /* _PROCARRAY_H_ */