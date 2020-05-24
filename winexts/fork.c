
#include "main.h"

/**
 * Credits to linux kernel developers
 */

DEFINE_TRACE(sched_process_fork);

void ignore_signal(int signal, struct task_struct* t)
{
    t->sighand->action[signal].sa.sa_handler = SIG_IGN;

    flush_signals(t);
}

void ignore_signals(struct task_struct* t)
{
    int i;

    for (i = 0; i < _NSIG; ++i)
        t->sighand->action[i].sa.sa_handler = SIG_IGN;

    flush_signals(t);
}

int wait_for_vfork_done(struct task_struct* child,
                        struct completion* vfork)
{
    int killed;

    freezer_do_not_count();
    cgroup_enter_frozen();
    killed = wait_for_completion_killable(vfork);
    cgroup_leave_frozen(false);
    freezer_count();

    if (killed)
    {
        task_lock(child);
        child->vfork_done = NULL;
        task_unlock(child);
    }

    put_task_struct(child);
    return killed;
}

/*
 *  Ok, this is the main fork-routine.
 *
 * It copies the process, and if successful kick-starts
 * it and waits for it to finish using the VM if required.
 *
 * args->exit_signal is expected to be checked for sanity by the caller.
 */
long c_do_fork(struct kernel_clone_args* args,
               communicate_regs_t* regs,
               communicate_regs_set_t* regs_set)
{
    u64 clone_flags = args->flags;
    struct completion vfork;
    struct pid* pid;
    struct task_struct* p;
    int trace = 0;
    long nr;
    int reg_index;
    struct pt_regs* pt_regs;

    /*
     * Determine whether and which event to report to ptracer.  When
     * called from kernel_thread or CLONE_UNTRACED is explicitly
     * requested, no event is reported; otherwise, report if the event
     * for the type of forking is enabled.
     */
    if (!(clone_flags & CLONE_UNTRACED))
    {
        if (clone_flags & CLONE_VFORK)
            trace = PTRACE_EVENT_VFORK;
        else if (args->exit_signal != SIGCHLD)
            trace = PTRACE_EVENT_CLONE;
        else
            trace = PTRACE_EVENT_FORK;

        if (likely(!ptrace_event_enabled(current, trace)))
            trace = 0;
    }

    p = copy_process(NULL, trace, NUMA_NO_NODE, args);

    add_latent_entropy();

    if (IS_ERR(p))
        return PTR_ERR(p);

    pt_regs = task_pt_regs(p);

    if (!IS_ERR(pt_regs))
    {
        for (reg_index = 0;
             reg_index < sizeof(communicate_regs_set_t) / sizeof(bool);
             reg_index++)
        {
            /* If register is asked to be set */
            if (*(bool*)((uintptr_t)regs_set + reg_index * sizeof(bool)))
            {
                *(unsigned long*)((uintptr_t)pt_regs
                                  + reg_index * sizeof(unsigned long))
                  = (unsigned long)*(uint64_t*)((uintptr_t)regs
                                                + reg_index
                                                    * sizeof(uint64_t));
            }
        }
    }

    /*
     * Do this prior waking up the new thread - the thread pointer
     * might get invalid after that point, if the thread exits quickly.
     */
    __tracepoint_sched_process_fork = *__tracepoint_sched_process_fork_ptr;
    trace_sched_process_fork(current, p);

    pid = get_task_pid(p, PIDTYPE_PID);
    nr  = pid_vnr(pid);

    if (clone_flags & CLONE_PARENT_SETTID)
        put_user(nr, args->parent_tid);

    if (clone_flags & CLONE_VFORK)
    {
        p->vfork_done = &vfork;
        init_completion(&vfork);
        get_task_struct(p);
    }

    wake_up_new_task(p);

    /* forking complete and child started to run, tell ptracer */
    if (unlikely(trace))
        ptrace_event_pid(trace, pid);

    if (clone_flags & CLONE_VFORK)
    {
        if (!wait_for_vfork_done(p, &vfork))
            ptrace_event_pid(PTRACE_EVENT_VFORK_DONE, pid);
    }

    put_pid(pid);

    return nr;
}
