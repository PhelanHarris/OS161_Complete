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

static bool vm_bootstrapped = false;
static paddr_t coremap_base;
static paddr_t coremap_end;
static unsigned coremap_numPages;


/* Initialization function */
void
vm_bootstrap (void)
{
	paddr_t first_addr, last_addr;
	int num_addr;

	// make sure this is only called once
	ASSERT(vm_bootstrapped == false);

	// get the first and last physical addresses
	first_addr = ram_getfirstfree();
	last_addr = ram_getsize();
	// put the coremap at the beginning of the physical memory
	coremap = (struct coremap_entry *) PADDR_TO_KVADDR(first_addr);
	
	// readjust where the first address is so that the coremap does not get
	// overwritten
	coremap_numPages = (last_addr - first_addr) / PAGE_SIZE;
	unsigned coremap_size = coremap_numPages * sizeof(struct coremap_entry);
	coremap_size = ROUNDUP(coremap_size, PAGE_SIZE);
	first_addr += coremap_size;
	coremap_numPages -= coremap_size / PAGE_SIZE;

	// initialize the coremap entries
	int i;
	for (i = 0; i < num_pages; i++) {
		coremap[i].as = NULL;
		coremap[i].va = NULL;
		coremap[i].block_size = 0;
		coremap[i].state = VM_STATE_FREE;
	}

	// update global values
	coremap_base = first_addr;
	coremap_end = last_addr;
	vm_bootstrapped = true;
}

/* Fault handling function called by trap code */
int
vm_fault (int faulttype, vaddr_t faultaddress)
{

}

/* Allocate/free kernel heap pages (called by kmalloc/kfree) */
vaddr_t
alloc_kpages(unsigned npages)
{
	paddr_t addr = 0;
	// if the vm hasn't bootstrapped, just steal memory
	if (vm_bootstrapped == false){
		spinlock_acquire(&stealmem_lock);
		addr = ram_stealmem(npages);
		spinlock_release(&stealmem_lock);

	} // otherwise, look for a contiguous chunk of memory in the coremap
	else {
		unsigned nfree = 0;
		unsigned curPage;
		for (curPage = 0; curPage < coremap_numPages; curPage++){
			if (coremap[curPage].state == VM_STATE_FREE){
				nfree++;
				if (nfree == npages){
					int i;
					for (i = curPage; i > curPage - npages; i--){
						coremap[i].state = VM_STATE_DIRTY;
						coremap[i].block_size = npages;
						// TODO: change other coremap entry fields
					}
					addr = ((curPage + 1 - npages) * PAGE_SIZE) + coremap_base;
					break;
				}
			}
			else{
				nfree = 0;
			}
		}
	}

	if (addr==0) {
		return 0;
	}
	return PADDR_TO_KVADDR(addr);
}

void
free_kpages(vaddr_t addr)
{
	paddr_t paddr = KVADDR_TO_PADDR(addr);

	unsigned cm_index = (paddr - coremap_base) / PAGE_SIZE;
	unsigned size = coremap[cm_index].block_size;
	int i;
	for (i = cm_index; i < size; i++){
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
vm_tlbshootdown(const struct tlbshootdown *)
{

}