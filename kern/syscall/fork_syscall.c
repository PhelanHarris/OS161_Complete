/*
 * Fork system call
 *
 */

#include <syscall.h>
#include <filetable.h>
#include <proctable.h>

int 
sys_fork(struct trapframe *tf, pid_t *pid_ret)
{
	// Todo
	int ret;
	struct addrspace *child_as;
	ret = as_copy(curproc->p_addrspace, &child_as);

	if (ret)
		return ret;

	struct trapframe *child_tf;
	child_tf = kmalloc(sizeof(*tf));

	if (child_tf == NULL)
		return ENOMEM;

	child_tf->tf_vaddr = tf->tf_vaddr;
	child_tf->tf_status = tf->tf_status;
	child_tf->tf_cause = tf->tf_cause;
	child_tf->tf_lo = tf->tf_lo;
	child_tf->tf_hi = tf->tf_hi;
	child_tf->tf_ra = tf->tf_ra;
	child_tf->tf_at = tf->tf_at;
	child_tf->tf_v0 = tf->tf_v0;
	child_tf->tf_v1 = tf->tf_v1;
	child_tf->tf_a0 = tf->tf_a0;
	child_tf->tf_a1 = tf->tf_a1;
	child_tf->tf_a2 = tf->tf_a2;
	child_tf->tf_a3 = tf->tf_a3;
	child_tf->tf_t0 = tf->tf_t0;
	child_tf->tf_t1 = tf->tf_t1;
	child_tf->tf_t2 = tf->tf_t2;
	child_tf->tf_t3 = tf->tf_t3;
	child_tf->tf_t4 = tf->tf_t4;
	child_tf->tf_t5 = tf->tf_t5;
	child_tf->tf_t6 = tf->tf_t6;
	child_tf->tf_t7 = tf->tf_t7;
	child_tf->tf_s0 = tf->tf_s0;
	child_tf->tf_s1 = tf->tf_s1;
	child_tf->tf_s2 = tf->tf_s2;
	child_tf->tf_s3 = tf->tf_s3;
	child_tf->tf_s4 = tf->tf_s4;
	child_tf->tf_s5 = tf->tf_s5;
	child_tf->tf_s6 = tf->tf_s6;
	child_tf->tf_s7 = tf->tf_s7;
	child_tf->tf_t8 = tf->tf_t8;
	child_tf->tf_t9 = tf->tf_t9;
	child_tf->tf_k0 = tf->tf_k0;	
	child_tf->tf_k1 = tf->tf_k1;
	child_tf->tf_gp = tf->tf_gp;
	child_tf->tf_sp = tf->tf_sp;
	child_tf->tf_s8 = tf->tf_s8;
	child_tf->tf_epc = tf->tf_epc;
	
}
