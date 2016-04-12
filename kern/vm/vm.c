#include <vm.h>

// coremap struct
struct coremap_entry **coremap;

/* Initialization function */
void vm_bootstrap(void){
	PADDR_TO_KVADDR();
}

/* Fault handling function called by trap code */
int vm_fault(int faulttype, vaddr_t faultaddress){

}

static
paddr_t
getppages(unsigned long npages)
{
	paddr_t addr;

	spinlock_acquire(&stealmem_lock);

	addr = ram_stealmem(npages);

	spinlock_release(&stealmem_lock);
	return addr;
}

/* Allocate/free kernel heap pages (called by kmalloc/kfree) */
vaddr_t alloc_kpages(unsigned npages){

}
void free_kpages(vaddr_t addr){

}

/* TLB shootdown handling called from interprocessor_interrupt */
void vm_tlbshootdown_all(void){

}
void vm_tlbshootdown(const struct tlbshootdown *){

}