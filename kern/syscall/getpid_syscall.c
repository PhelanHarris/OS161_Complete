/*
 * Get the current process ID
 *
 */

 int
 sys_getpid(pid_t *pid_ret)
 {
 	return curproc->p_id;
 }