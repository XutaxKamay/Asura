#include "main.h"

int c_wait_for_vfork_done(struct task_struct* child,
                          struct completion* vfork)
{
    int killed;

    freezer_do_not_count();
    killed = wait_for_completion_killable(vfork);
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

long c_do_fork(task_t* task,
               struct kernel_clone_args* args,
               struct pt_regs* regs,
               communicate_regs_set_t* regs_set)
{
    u64 clone_flags = args->flags;
    struct completion vfork;
    struct pid* pid;
    struct task_struct *p, *old_current, **cur_task_ptr;
    struct pt_regs* p_regs;
    int trace = 0;
    int reg_index;
    long nr;
    static DEFINE_SPINLOCK(spinlock);

    if (!(clone_flags & CLONE_UNTRACED))
    {
        if (clone_flags & CLONE_VFORK)
            trace = PTRACE_EVENT_VFORK;
        else if (args->exit_signal != SIGCHLD)
            trace = PTRACE_EVENT_CLONE;
        else
            trace = PTRACE_EVENT_FORK;

        if (likely(!ptrace_event_enabled(task, trace)))
            trace = 0;
    }

    old_current = get_current();

    /**
     * Now let's trick the kernel.
     */

    spin_lock(&spinlock);

    cur_task_ptr  = get_current_task_ptr();
    *cur_task_ptr = task;
    this_cpu_write(cpu_current_top_of_stack, task_top_of_stack(task));
    update_task_stack(task);

    p = copy_process(NULL, trace, NUMA_NO_NODE, args);

    if (!IS_ERR(p) && p)
    {
        p_regs = task_pt_regs(p);

        if (!IS_ERR(p_regs) && p_regs)
        {
            for (reg_index = 0; reg_index < sizeof(communicate_regs_set_t)
                                              / sizeof(bool);
                 reg_index++)
            {
                /* If register is asked to be set */
                if (*(bool*)((uintptr_t)regs_set
                             + reg_index * sizeof(bool)))
                {
                    *(unsigned long*)((uintptr_t)p_regs
                                      + reg_index * sizeof(unsigned long))
                      = *(unsigned long*)((uintptr_t)regs
                                          + reg_index
                                              * sizeof(unsigned long));
                }
            }
        }
    }

    cur_task_ptr  = get_current_task_ptr();
    *cur_task_ptr = old_current;
    this_cpu_write(cpu_current_top_of_stack, task_top_of_stack(old_current));
    update_task_stack(old_current);

    spin_unlock(&spinlock);

    /**
     * Should be good now
     */

    add_latent_entropy();

    if (IS_ERR(p))
    {
        return PTR_ERR(p);
    }

    // TODO:
    // We might implement this for the scheduler, but it's just tracing
    // informations trace_sched_process_fork(task, p);

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

    if (unlikely(trace))
        c_ptrace_event_pid(task, trace, pid);

    if (clone_flags & CLONE_VFORK)
    {
        if (!c_wait_for_vfork_done(p, &vfork))
            c_ptrace_event_pid(task, PTRACE_EVENT_VFORK_DONE, pid);
    }

    put_pid(pid);

    return nr;
}
