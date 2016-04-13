#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <spl.h>
#include <spinlock.h>
#include <proc.h>
#include <current.h>
#include <mips/tlb.h>
#include <addrspace.h>
#include <vm.h>

// coremap struct
struct coremap_entry *coremap;


/* Initialization function */
void vm_bootstrap(void){
	paddr_t first_addr, last_addr;
	int num_addr;

	// get the first and last physical addresses
	first_addr = ram_getfirstfree();
	last_addr = ram_getsize();
	// put the coremap at the beginning of the physical memory
	coremap = (struct coremap_entry *) PADDR_TO_KVADDR(first_addr);
	
	// readjust where the first address is so that the coremap does not get
	// overwritten
	num_pages = (last_addr - first_addr) / PAGE_SIZE;
	unsigned coremap_size = num_pages * sizeof(struct coremap_entry);
	coremap_size = ROUNDUP(coremap_size, PAGE_SIZE);
	first_addr += coremap_size;
	num_pages -= coremap_size / PAGE_SIZE;

	// initialize the coremap entries
	int i;
	for (i = 0; i < num_pages; i++){
		coremap[i].as = NULL;
		coremap[i].va = NULL;
		coremap[i].state = VM_STATE_FREE;
	}
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