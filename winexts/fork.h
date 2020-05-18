#ifndef FORK_H
#define FORK_H

/**
 * If anyone in the linux kernel development is reading this...
 * I would like to thank you for these codes that helped me for my
 * project.
 */

/**
 * This needs to be maintained if there's any updates inside the kernel
 * for kernel/fork.c
 */

DECLARE_PER_CPU(unsigned long, process_counts);

void c_proc_caches_init(void);

int c_wait_for_vfork_done(struct task_struct* child,
                          struct completion* vfork);
long c_do_fork(task_t* task,
               struct kernel_clone_args* args,
               struct pt_regs* regs,
               communicate_regs_set_t* regs_set);

#endif
