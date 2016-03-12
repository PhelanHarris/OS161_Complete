#ifndef _PROCARRAY_H_
#define _PROCARRAY_H_

#include <types.h>
#include <proc.h>
#include <synch.h>

struct proctable_entry {
	struct proc *pte_p;
	bool pte_running;
	int pte_exitcode;
	int pte_refcount;
	struct cv *pte_cv;
	struct lock *pte_lock;
};

struct proctable {
	struct proctable_entry **pt_v;
	unsigned pt_num, pt_size;
};

struct spinlock proctable_lock;

extern struct proctable *proctable;

void proctable_init(void);
int proctable_add(struct proc *p, pid_t *ret_pid);
struct proctable_entry *proctable_get(pid_t pid);
bool proctable_remove(pid_t pid);

#endif /* _PROCARRAY_H_ */