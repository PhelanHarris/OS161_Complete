/*
 * Fork system call
 *
 */

#include <types.h>
#include <proc.h>
#include <current.h>
#include <syscall.h>
#include <filetable.h>
#include <proctable.h>
#include <mips/trapframe.h>
#include <kern/errno.h>
#include <addrspace.h>

int
sys_execv(const char *program, userptr_t args)
{
	struct addrspace *as;
	struct vnode *v;
	vaddr_t entrypoint, stackptr;
	int result;

	return 0;
}
