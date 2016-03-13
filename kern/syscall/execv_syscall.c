/*
 * Fork system call
 *
 */

#include <types.h>
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

int
sys_execv(const char *program, char **args)
{
	struct addrspace *as, *oldas;
	struct vnode *v;
	vaddr_t entrypoint, stackptr;
	int result;
	int argc = 0;
	char *kprogram;
	size_t programlen;
	kprogram = kmalloc(sizeof(char) * (strlen(program) + 1));

	// copy program name into kernel space
	result = copyinstr((const_userptr_t)program, kprogram, sizeof(char) * (strlen(program) + 1), &programlen);
	if (result){
		return result;
	}

	// open the file
	result = vfs_open((char*)program, O_RDONLY, 0, &v);
	if (result){
		return result;
	}

	// create new address space
	as = as_create();
	if (as == NULL){
		vfs_close(v);
		return ENOMEM;
	}

	// save a pointer to the old address space to destroy later
	oldas = proc_getas();
	as_deactivate();

	// switch to new address space and activate it
	proc_setas(as);
	as_activate();

	// load the executable
	result = load_elf(v, &entrypoint);
	if (result){
		// destroy the new address space
		as_deactivate();
		as_destroy(as);
		// reactivate the old address space before returning
		proc_setas(oldas);
		as_activate();
		vfs_close(v);
		return result;
	}

	// done with file now
	vfs_close(v);

	// define the user stack in the address space
	result = as_define_stack(as, &stackptr);
	if (result){
		// destroy the new address space
		as_deactivate();
		as_destroy(as);
		// reactivate the old address space before returning
		proc_setas(oldas);
		as_activate();
		return result;
	}

	size_t arglen = 0;
	argc = 0;

	while (args[argc] != NULL && arglen < ARG_MAX){
		arglen += strlen(args[argc]) + 1;
		argc++;
	}

	if (arglen >= ARG_MAX){
		// destroy the new address space
		as_deactivate();
		as_destroy(as);
		// reactivate the old address space before returning
		proc_setas(oldas);
		as_activate();
		return E2BIG;
	}

	char *kargs[argc + 1];
	userptr_t arg_ptrs[argc + 1];
	size_t arglengths[argc];
	int total_len = 0;
	size_t actual = 0;
	int i;
	for (i = 0; i < argc; i++){
		
		kargs[i] = kmalloc(sizeof(char) * (strlen(args[i]) + 1));
		result = copyinstr((const_userptr_t)args[i], kargs[i], strlen(args[i]) + 1, &actual);
		if (result){
			// destroy the new address space
			as_deactivate();
			as_destroy(as);
			// reactivate the old address space before returning
			proc_setas(oldas);
			as_activate();
			// free all the strings
			int j;
			for (j = 0; j < i; j++){
				kfree(kargs[j]);
			}
			kfree(kargs);	
			return result;
		}
		
		arglengths[i] = actual;
		if (actual % 4 == 0) {
			total_len += actual;
		}
		else {
			total_len += actual + (4 - actual % 4);
		}
	}
	// null terminate the array of args
	kargs[argc] = NULL;

	// make space on the stack for arguments
	stackptr -= (argc+1)*4 + total_len;

	userptr_t arg_dest = (userptr_t) stackptr;
	userptr_t argval_dest = (userptr_t) stackptr + argc*4;



	// copy out string arguments back to user space
	for (i = 0; i < argc; i++){
		// copy out the string array values
		copyoutstr(kargs[i], argval_dest, arglengths[i], &actual);
		arg_ptrs[i] = (userptr_t) argval_dest;
		
		// copy out the pointer to the string array values
		copyout(&arg_ptrs[i], arg_dest, sizeof(char*));
		arg_dest += sizeof(char*);

		// increment the argval_dest by the length of the last argument (padded)
		if (arglengths[i] % 4 == 0){
			argval_dest += arglengths[i];
		}
		else {
			argval_dest += arglengths[i] + (4 - arglengths[i] % 4);
		}
	}

	/* Warp to user mode. */
	enter_new_process(argc /*argc*/, (userptr_t)args /*userspace addr of argv*/,
			  NULL /*userspace addr of environment*/,
			  stackptr, entrypoint);
	
	panic("In execv: enter_new_process returned! :(\n");
	return 0;
}
