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
#include <test.h>

int
sys_execv(const char *program, char **args)
{
	struct addrspace *oldas = proc_getas();
	int result;
	int argc = 0;
	char *kprogram = kmalloc(sizeof(char) * (strlen(program) + 1));
	size_t programlen;

	// copy program name into kernel space
	result = copyinstr((const_userptr_t)program, kprogram, sizeof(char) * (strlen(program) + 1), &programlen);
	if (result){
		return result;
	}

	size_t arg_space = 0;
	argc = 0;

	while (args[argc] != NULL && arg_space < ARG_MAX){
		size_t cur_arglen = strlen(args[argc]) + 1;
		// padded length
		arg_space += cur_arglen + ((4 - cur_arglen % 4) % 4);
		argc++;
	}
	arg_space += (argc+1)*sizeof(char*);
	
	if (arg_space >= ARG_MAX){
		kfree(kprogram);
		return E2BIG;
	}

	char *kargs[argc + 1];
	size_t actual = 0;
	int i;
	for (i = 0; i < argc; i++){
		
		kargs[i] = kmalloc(sizeof(char) * (strlen(args[i]) + 1));
		result = copyinstr((const_userptr_t)args[i], kargs[i], strlen(args[i]) + 1, &actual);
		if (result){
			kfree(kprogram);
			int j;
			for (j = 0; j < i; j++){
				kfree(kargs[j]);
			}
			return result;
		}
	}
	// null terminate the array of args
	kargs[argc] = NULL;

	// pass the arguments to runprogram now that 
	// they have been copied to kernel space.
	runprogram((char *)kprogram, argc, kargs, oldas);
	
	panic("In execv: enter_new_process returned! :(\n");
	return 0;
}
