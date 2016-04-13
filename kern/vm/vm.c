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

// Coremap struct
struct coremap_entry *coremap;

// Address bounds of memory
paddr_t mem_first_addr;
paddr_t mem_last_addr;

// Address bounds of coremap
paddr_t coremap_base;
paddr_t coremap_end;

// Coremap dimensions
unsigned coremap_num_pages;
unsigned coremap_size;

// Synchronizing stuff
struct spinlock stealmem_lock = SPINLOCK_INITIALIZER;
struct lock *coremap_lock;

// Bootstrap status
bool vm_bootstrapped = false;


static
int
get_cm_index_by_paddr(paddr_t paddr)
{
	int index = (paddr - mem_first_addr) / PAGE_SIZE;
	KASSERT(index >= 0);
	return index;
}

static
paddr_t
get_paddr_by_cm_index(int index)
{
	paddr_t paddr = mem_first_addr + (index * PAGE_SIZE);
	KASSERT(paddr >= mem_first_addr);
	return paddr;
}

/* Initialization function */
void
vm_bootstrap (void)
{
	/* Make sure this is only called once */
	KASSERT(vm_bootstrapped == false);

	/* Initalize locks */
	coremap_lock = lock_create("vm_coremap");

	/* Get the pointer to the size of the ram */
	mem_first_addr = KVADDR_TO_PADDR(firstfree);
	mem_last_addr = ram_getsize();
	coremap_base = ram_getfirstfree();

	/* Put the coremap at the beginning of the physical memory */
	coremap = (struct coremap_entry *) PADDR_TO_KVADDR(coremap_base);
	coremap_num_pages = (coremap_base - mem_first_addr) / PAGE_SIZE;
	
	/* Compute size of coremap */
	coremap_size = ROUNDUP(coremap_num_pages * sizeof(struct coremap_entry), PAGE_SIZE);
	coremap_end = coremap_base + coremap_size;

	/* Get index of coremap's page */
	unsigned cmi_start = get_cm_index_by_paddr(coremap_base);
	unsigned cmi_end = get_cm_index_by_paddr(coremap_end);

	/* Initialize the coremap entries, one for each physical page */
	unsigned i;
	for (i = 0; i < coremap_num_pages; i++) {
		coremap[i].block_size = 0;

		/* Stolen memory is 'dirty', coremap region is 'fixed' */
		if (i < cmi_start) {
			coremap[i].state = VM_STATE_DIRTY;
		} else if (i <= cmi_end) {
			coremap[i].state = VM_STATE_FIXED;
		} else {
			coremap[i].state = VM_STATE_FREE;
		}
	}

	/* Bootstrap complete */
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
	 	/* Look for a contiguous chunk of memory in the coremap */
		lock_acquire(coremap_lock);

		unsigned nfree = 0;
		unsigned cur_page;
		for (cur_page = 0; cur_page < coremap_num_pages; cur_page++) {
			if (coremap[cur_page].state == VM_STATE_FREE){
				nfree++;
				if (nfree == npages) {
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

	/* If an address was found, convert to a kvaddr */
	return addr == 0 ? 0 : PADDR_TO_KVADDR(addr);
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