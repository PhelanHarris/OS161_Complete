#ifndef _PROCARRAY_H_
#define _PROCARRAY_H_

#include <proc.h>

void proctable_init(struct array *pt);
pid_t proctable_add(struct array *pt, proc* p);
void proctable_remove(struct array *pt, pid_t pid);

#endif /* _PROCARRAY_H_ */