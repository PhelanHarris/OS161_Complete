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
	char *kprogram;
	size_t programlen;

	if (program == NULL){
		return ENOENT;
	}
	else {
		kprogram = kmalloc(sizeof(char) * (strlen(program) + 1));
		if (kprogram == NULL){
			return ENOMEM;
		}
		// copy program name into kernel space
		result = copyinstr((const_userptr_t)program, kprogram, sizeof(char) * (strlen(program) + 1), &programlen);
		if (result){
			kfree(kprogram);
			return result;
		}
	}

	size_t total_arglen = 0;
	while (args[argc] != NULL){
		total_arglen += strlen(args[argc]) + 1;
		argc++;
	}
	if (total_arglen > ARG_MAX){
		kfree(kprogram);
		return E2BIG;
	}

	char *kargs[argc];
	size_t actual = 0;
	int i;
	for (i = 0; i < argc; i++){
		
		kargs[i] = kmalloc(sizeof(char) * (strlen(args[i]) + 1));
		if (kargs[i] == NULL){
			kfree(kprogram);
			int j;
			for (j = 0; j < i; j++){
				kfree(kargs[j]);
			}
			return ENOMEM;
		}
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

	// pass the arguments to runprogram now that 
	// they have been copied to kernel space.
	result = runprogram((char *)kprogram, argc, kargs, oldas);
	if (result){
		return result;
	}

	panic("In execv: runprogram returned! :(\n");
	return 0;
}
