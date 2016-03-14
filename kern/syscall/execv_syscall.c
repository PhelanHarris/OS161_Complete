/*
 * Fork system call
 *
 */

#include <types.h>
#include <limits.h>
#include <kern/fcntl.h>
#include <proc.h>
#include <current.h>
#include <syscall.h>
#include <filetable.h>
#include <proctable.h>
#include <mips/trapframe.h>
#include <kern/errno.h>
#include <addrspace.h>
#include <vfs.h>
#include <copyinout.h>
#include <test.h>

int
sys_execv(const char *program, char **args)
{
	struct addrspace *as, *oldas = proc_getas();
	vaddr_t entrypoint, stackptr;
	struct vnode *v;
	size_t programlen;
	char *kprogram;
	int total_arglen = 0;
	int result;
	int argc = 0;
	int i;

	// Check for null pointers
	if (program == NULL || args == NULL) {
		return EFAULT;
	}
	
	// Allocate program name
	kprogram = kmalloc(sizeof(char) * PATH_MAX);
	if (kprogram == NULL) {
		return ENOMEM;
	}

	// copy program name into kernel space
	result = copyinstr((const_userptr_t)program, kprogram, sizeof(char) * PATH_MAX, &programlen);
	if (result) {
		kfree(kprogram);
		return result;
	}

	// Allocate args
	char *kargs = kmalloc(sizeof(char) * ARG_MAX);
	if (kargs == NULL){
		kfree(kprogram);
		return ENOMEM;
	}
	size_t *arglengths = kmalloc(sizeof(size_t) * ARG_MAX);
	if (arglengths == NULL) {
		kfree(kargs);
		kfree(kprogram);
		return ENOMEM;
	}
		
	// Copy args to kernel space
	size_t actual = 0;
	do {
		result = copyinstr((userptr_t) *(args+argc), kargs + total_arglen,
			ARG_MAX - total_arglen, &actual);

		if (result) {
			kfree(arglengths);
			kfree(kargs);
			kfree(kprogram);
			return result;
		}

		arglengths[argc] = actual + ((4 - actual % 4) % 4);
		total_arglen += arglengths[argc];
		argc++;
	} while (*(args+argc) != NULL && total_arglen < ARG_MAX);

	// Open the file
	result = vfs_open(kprogram, O_RDONLY, 0, &v);
	if (result) {
		kfree(arglengths);
		kfree(kargs);
		kfree(kprogram);
		return result;
	}

	// Create a new address space
	as = as_create();
	if (as == NULL) {
		vfs_close(v);
		kfree(arglengths);
		kfree(kargs);
		kfree(kprogram);
		return result;
	}

	// Switch and activate new addrspace
	as_deactivate();
	proc_setas(as);
	as_activate();

	// Load executable
	result = load_elf(v, &entrypoint);
	if (result) {
		// destroy the new address space
		as_deactivate();
		as_destroy(as);
		// reactivate the old address space before returning
		proc_setas(oldas);
		as_activate();
		vfs_close(v);
		kfree(arglengths);
		kfree(kargs);
		kfree(kprogram);
		return result;
	}

	// Done with the file now.
	vfs_close(v);

	// Define the user stack in the address space
	result = as_define_stack(as, &stackptr);
	if (result) {
		as_deactivate();
		as_destroy(as);
		proc_setas(oldas);
		as_activate();
		kfree(arglengths);
		kfree(kargs);
		kfree(kprogram);
		return result;
	}

	// Make space on the stack for arguments
	stackptr -= (argc+1)*4 + total_arglen;

	// Copy out entire string of arguments
	userptr_t argval_dest = (userptr_t) stackptr + (argc+1)*4;
	copyoutstr(kargs, argval_dest, total_arglen, &actual);

	// Copy out addresses of each string argument
	userptr_t arg_dest = (userptr_t) stackptr;
	for (i = 0; i < argc; i++) {
		copyout(&argval_dest, arg_dest, sizeof(char*));
		arg_dest += sizeof(char*);

		// increment the argval_dest by the length of the last argument (padded)
		argval_dest += arglengths[i];
	}

	// Copy out null terminator
	argval_dest = NULL;
	copyout(&argval_dest, arg_dest, sizeof(char*));

	// Cleanup
	as_destroy(oldas);
	kfree(arglengths);
	kfree(kargs);
	kfree(kprogram);

	// Warp to user mode
	enter_new_process(argc, (userptr_t) stackptr, NULL, stackptr, entrypoint);

	// enter_new_process does not return.
	panic("Execv: enter_new_process returned! :(\n");
	return EINVAL;
}
