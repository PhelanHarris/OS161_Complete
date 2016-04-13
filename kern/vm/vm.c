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
#include <synch.h>

// coremap struct
struct coremap_entry *coremap;

bool vm_bootstrapped = false;
paddr_t coremap_base;
paddr_t coremap_end;
unsigned coremap_num_pages;

struct spinlock stealmem_lock = SPINLOCK_INITIALIZER;
struct lock *coremap_lock;


/* Initialization function */
void
vm_bootstrap (void)
{
	paddr_t first_addr, last_addr;

	/* Make sure this is only called once */
	KASSERT(vm_bootstrapped == false);

	/* Initalize locks */
	coremap_lock = lock_create("vm_coremap");

	/* Get the first and last physical addresses */
	first_addr = ram_getfirstfree();
	last_addr = ram_getsize();

	/* Put the coremap at the beginning of the physical memory */
	coremap = (struct coremap_entry *) PADDR_TO_KVADDR(first_addr);
	
	/* Re-adjust where the first address is so that the coremap does not get
	   overwritten */
	coremap_num_pages = (last_addr - first_addr) / PAGE_SIZE;
	unsigned coremap_size = coremap_num_pages * sizeof(struct coremap_entry);
	coremap_size = ROUNDUP(coremap_size, PAGE_SIZE);
	first_addr += coremap_size;
	coremap_num_pages -= coremap_size / PAGE_SIZE;

	/* Initialize the coremap entries, one for each physical page */
	unsigned i;
	for (i = 0; i < coremap_num_pages; i++) {
		coremap[i].block_size = 0;
		coremap[i].state = VM_STATE_FREE;
	}

	/* Update global values */
	coremap_base = first_addr;
	coremap_end = last_addr;
	vm_bootstrapped = true;
}

/* Fault handling function called by trap code */
int
vm_fault (int faulttype, vaddr_t faultaddress)
{
	(void) faulttype;
	(void) faultaddress;
	return ENOSYS;
}

int
get_cm_index_by_paddr(paddr_t paddr)
{
	int index = (paddr - coremap_base) / PAGE_SIZE;
	KASSERT(index >= 0);
	return index;
}

paddr_t
get_paddr_by_cm_index(int index)
{
	paddr_t paddr = coremap_base + (index * PAGE_SIZE);
	KASSERT(paddr >= coremap_base);
	return paddr;
}


/* Allocate/free kernel heap pages (called by kmalloc/kfree) */
vaddr_t
alloc_kpages(unsigned npages)
{
	paddr_t addr = 0;
	/* If the vm hasn't bootstrapped, just steal memory */
	if (vm_bootstrapped == false){
		spinlock_acquire(&stealmem_lock);
		addr = ram_stealmem(npages);
		spinlock_release(&stealmem_lock);

	} else {
	 	/* look for a contiguous chunk of memory in the coremap */
		lock_acquire(coremap_lock);
		unsigned nfree = 0;
		unsigned cur_page;
		for (cur_page = 0; cur_page < coremap_num_pages; cur_page++) {
			if (coremap[cur_page].state == VM_STATE_FREE){
				nfree++;
				if (nfree == npages ){
					unsigned i;
					for (i = cur_page; i > cur_page - npages; i--) {
						coremap[i].state = VM_STATE_DIRTY;
						coremap[i].block_size = npages;
					}
					addr = get_paddr_by_cm_index(cur_page + 1 - npages);
					break;
				}
			} else {
				nfree = 0;
			}
		}

		lock_release(coremap_lock);
	}

	if (addr==0) {
		return 0;
	}
	return PADDR_TO_KVADDR(addr);
}

void
free_kpages(vaddr_t vaddr)
{
	int cm_index = get_cm_index_by_paddr(KVADDR_TO_PADDR(vaddr));

	int size = coremap[cm_index].block_size;
	int i;
	for (i = cm_index; i < size; i++) {
		KASSERT(coremap[i].state != VM_STATE_FIXED)
		coremap[i].state = VM_STATE_FREE;
		coremap[i].block_size = 0;
	}
}

/* TLB shootdown handling called from interprocessor_interrupt */
void 
vm_tlbshootdown_all(void)
{

}

void
vm_tlbshootdown(const struct tlbshootdown *arg)
{	
	(void) arg;
}