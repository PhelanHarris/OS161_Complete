/*
 * Fork system call
 *
 */

#include <syscall.h>
#include <filetable.h>
#include <proctable.h>

int 
sys_fork(pid_t *pid_ret)
{
	// Todo
	int ret;
	struct addrspace *child_as;
	ret = as_copy(curproc->p_addrspace, &child_as);

	if (ret)
		return ret;

	
}
