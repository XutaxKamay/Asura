#ifndef PTRACE_EVENT_PID_H
#define PTRACE_EVENT_PID_H

#include "main.h"

/**
 * If anyone in the linux kernel development is reading this...
 * I would like to thank you for these codes that helped me for my project.
 */

/**
 * This needs to be maintained if there's any updates inside the kernel
 * for kernel/signal.c
 */

#define PENDING(p, b) c_has_pending_signals(&(p)->signal, (b))

int c_wait_for_vfork_done(struct task_struct* child,
                         struct completion* vfork);
void c_cgroup_inc_frozen_cnt(struct cgroup* cgrp);
void c_cgroup_dec_frozen_cnt(struct cgroup* cgrp);
void c_cgroup_enter_frozen(struct task_struct* task);
void c_cgroup_leave_frozen(struct task_struct* task, bool always_leave);
bool c_task_participate_group_stop(struct task_struct* task);
bool c_sigkill_pending(struct task_struct* tsk);
bool c_may_ptrace_stop(struct task_struct* task);
void c_do_notify_parent_cldstop(struct task_struct* tsk,
                                bool for_ptracer,
                                int why);
bool c_has_pending_signals(sigset_t* signal, sigset_t* blocked);
bool c_recalc_sigpending_tsk(struct task_struct* t);
void c_ptrace_stop(task_t* task,
                   int exit_code,
                   int why,
                   int clear_code,
                   kernel_siginfo_t* info);
void c_ptrace_do_notify(task_t* task, int signr, int exit_code, int why);
void c_ptrace_notify(task_t* task, int exit_code);
void c_ptrace_event(task_t* task, int event, unsigned long message);
void c_ptrace_event_pid(task_t* task, int event, struct pid* pid);

#endif
