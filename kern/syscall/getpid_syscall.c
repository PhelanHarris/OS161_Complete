/*
 * Get the current process ID
 *
 */

#include <types.h>
#include <proc.h>
#include <current.h>

 int
 sys_getpid(pid_t *pid_ret)
 {
 	return curproc->p_id;
 }