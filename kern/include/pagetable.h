#ifndef _PAGETABLE_H_
#define _PAGETABLE_H_

#define PAGETABLE_SIZE 1024
#define PAGETABLE_L1 0
#define PAGETABLE_L2 1

struct pagetable_entry {
	paddr_t paddr;
	bool mapped;
};

struct pagetable_entry *pagetable_create();
void pagetable_destory(pagetable_entry *pt);
void pagetable_get_entry(struct addrspace *as, vaddr_t vaddr);

#endif /* _PAGETABLE_H_ */