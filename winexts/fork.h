
#ifndef FORK_H
#define FORK_H

void ignore_signal(int signal, struct task_struct* t);

int wait_for_vfork_done(struct task_struct* child,
                        struct completion* vfork);

long c_do_fork(struct kernel_clone_args* args,
               communicate_regs_t* regs,
               communicate_regs_set_t* regs_set);

#endif
