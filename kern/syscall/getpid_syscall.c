/*
 * Get the current process ID
 *
 */

#include <types.h>
#include <proc.h>
#include <current.h>
#include <syscall.h>

 int
 sys_getpid(pid_t *pid_ret)
 {
  	*pid_ret = curproc->p_id;
 	return 0;
 }