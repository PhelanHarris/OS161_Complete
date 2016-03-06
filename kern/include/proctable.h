#ifndef _PROCARRAY_H_
#define _PROCARRAY_H_

#include <proc.h>

struct proctable {
	proc **v;
	unsigned num, size, max;
};

struct proctable * proctable_create(void);
pid_t proctable_add(struct proctable *pt, proc* p);
void proctable_remove(struct proctable *pt, pid_t pid);

#endif /* _PROCARRAY_H_ */