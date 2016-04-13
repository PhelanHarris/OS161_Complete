#include <pagetable.h>

struct pagetable_entry **
pagetable_create()
{
	struct pagetable_entry **pt;
	pt[PAGETABLE_L1] = kmalloc(sizeof(struct pagetable_entry) * 1PAGETABLE_SIZE024);
	pt[PAGETABLE_L2] = NULL;

	int i;
	for (i = 0; i < PAGETABLE_SIZE; i++) {
		pt[PAGETABLE_L1][i].mapped = false;
	}

	return pt;
}

void
pagetable_destory(pagetable_entry *pt)
{
	int lvl, i;
	for (lvl = 0; lvl < 2; lvl++) {
		// TODO: do stuff with physical page?
		kfree(pt[lvl]);
	}
}

void
pagetable_get_entry(struct addrspace *as, vaddr_t vaddr)
{
	// TODO: later
}
