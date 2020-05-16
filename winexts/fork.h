#ifndef FORK_H
#define FORK_H

int c_wait_for_vfork_done(struct task_struct* child,
                          struct completion* vfork);
long c_do_fork(task_t* task,
               struct kernel_clone_args* args,
               struct pt_regs* regs,
               communicate_regs_set_t* regs_set);

#endif
