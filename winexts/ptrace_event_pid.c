#include "main.h"

/**
 * TODO:
 * check the end of mem_utils.c
 */
// static DEFINE_SPINLOCK(new_css_set_lock);
// static DEFINE_RWLOCK(new_tasklist_lock);

/**
 * If anyone in the linux kernel development is reading this...
 * I would like to thank you for these codes that helped me for my
 * project.
 */

/**
 * This needs to be maintained if there's any updates inside the kernel
 * for kernel/signal.c
 */

/*
 * Increment cgroup's nr_frozen_tasks.
 */
void c_cgroup_inc_frozen_cnt(struct cgroup* cgrp)
{
    cgrp->freezer.nr_frozen_tasks++;
}

/*
 * Decrement cgroup's nr_frozen_tasks.
 */
void c_cgroup_dec_frozen_cnt(struct cgroup* cgrp)
{
    cgrp->freezer.nr_frozen_tasks--;
    WARN_ON_ONCE(cgrp->freezer.nr_frozen_tasks < 0);
}

/*
 * Enter frozen/stopped state, if not yet there. Update cgroup's counters,
 * and revisit the state of the cgroup, if necessary.
 */
void c_cgroup_enter_frozen(struct task_struct* task)
{
    struct cgroup* cgrp;

    if (task->frozen)
        return;

    spin_lock_irq(pcss_set_lock);
    task->frozen = true;
    cgrp         = task_dfl_cgroup(task);
    c_cgroup_inc_frozen_cnt(cgrp);
    cgroup_update_frozen(cgrp);
    spin_unlock_irq(pcss_set_lock);
}

/*
 * Conditionally leave frozen/stopped state. Update cgroup's counters,
 * and revisit the state of the cgroup, if necessary.
 *
 * If always_leave is not set, and the cgroup is freezing,
 * we're racing with the cgroup freezing. In this case, we don't
 * drop the frozen counter to avoid a transient switch to
 * the unfrozen state.
 */
void c_cgroup_leave_frozen(struct task_struct* task, bool always_leave)
{
    struct cgroup* cgrp;

    spin_lock_irq(pcss_set_lock);
    cgrp = task_dfl_cgroup(task);
    if (always_leave || !test_bit(CGRP_FREEZE, &cgrp->flags))
    {
        c_cgroup_dec_frozen_cnt(cgrp);
        cgroup_update_frozen(cgrp);
        WARN_ON_ONCE(!task->frozen);
        task->frozen = false;
    }
    else if (!(task->jobctl & JOBCTL_TRAP_FREEZE))
    {
        spin_lock(&task->sighand->siglock);
        task->jobctl |= JOBCTL_TRAP_FREEZE;
        set_thread_flag(TIF_SIGPENDING);
        spin_unlock(&task->sighand->siglock);
    }
    spin_unlock_irq(pcss_set_lock);
}

bool c_task_participate_group_stop(struct task_struct* task)
{
    struct signal_struct* sig = task->signal;
    bool consume              = task->jobctl & JOBCTL_STOP_CONSUME;

    WARN_ON_ONCE(!(task->jobctl & JOBCTL_STOP_PENDING));

    task_clear_jobctl_pending(task, JOBCTL_STOP_PENDING);

    if (!consume)
        return false;

    if (!WARN_ON_ONCE(sig->group_stop_count == 0))
        sig->group_stop_count--;

    /*
     * Tell the caller to notify completion iff we are entering into a
     * fresh group stop.  Read comment in do_signal_stop() for details.
     */
    if (!sig->group_stop_count && !(sig->flags & SIGNAL_STOP_STOPPED))
    {
        signal_set_stop_flags(sig, SIGNAL_STOP_STOPPED);
        return true;
    }
    return false;
}

bool c_sigkill_pending(struct task_struct* tsk)
{
    return sigismember(&tsk->pending.signal, SIGKILL)
           || sigismember(&tsk->signal->shared_pending.signal, SIGKILL);
}

inline bool c_may_ptrace_stop(struct task_struct* task)
{
    if (!likely(task->ptrace))
        return false;
    /*
     * Are we in the middle of do_coredump?
     * If so and our tracer is also part of the coredump stopping
     * is a deadlock situation, and pointless because our tracer
     * is dead so don't allow us to stop.
     * If SIGKILL was already sent before the caller unlocked
     * ->siglock we must see ->core_state != NULL. Otherwise it
     * is safe to enter schedule().
     *
     * This is almost outdated, a task with the pending SIGKILL can't
     * block in TASK_TRACED. But PTRACE_EVENT_EXIT can be reported
     * after SIGKILL was already dequeued.
     */
    if (unlikely(task->mm->core_state)
        && unlikely(task->mm == task->parent->mm))
        return false;

    return true;
}

void c_do_notify_parent_cldstop(struct task_struct* tsk,
                                bool for_ptracer,
                                int why)
{
    struct kernel_siginfo info;
    unsigned long flags;
    struct task_struct* parent;
    struct sighand_struct* sighand;
    u64 utime, stime;

    if (for_ptracer)
    {
        parent = tsk->parent;
    }
    else
    {
        tsk    = tsk->group_leader;
        parent = tsk->real_parent;
    }

    clear_siginfo(&info);
    info.si_signo = SIGCHLD;
    info.si_errno = 0;
    /*
     * see comment in do_notify_parent() about the following 4 lines
     */
    rcu_read_lock();
    info.si_pid = task_pid_nr_ns(tsk, task_active_pid_ns(parent));
    info.si_uid = from_kuid_munged(task_cred_xxx(parent, user_ns),
                                   task_uid(tsk));
    rcu_read_unlock();

    task_cputime(tsk, &utime, &stime);
    info.si_utime = nsec_to_clock_t(utime);
    info.si_stime = nsec_to_clock_t(stime);

    info.si_code = why;
    switch (why)
    {
        case CLD_CONTINUED:
            info.si_status = SIGCONT;
            break;
        case CLD_STOPPED:
            info.si_status = tsk->signal->group_exit_code & 0x7f;
            break;
        case CLD_TRAPPED:
            info.si_status = tsk->exit_code & 0x7f;
            break;
        default:
            BUG();
    }

    sighand = parent->sighand;
    spin_lock_irqsave(&sighand->siglock, flags);
    if (sighand->action[SIGCHLD - 1].sa.sa_handler != SIG_IGN
        && !(sighand->action[SIGCHLD - 1].sa.sa_flags & SA_NOCLDSTOP))
        __group_send_sig_info(SIGCHLD, &info, parent);
    /*
     * Even if SIGCHLD is not generated, we must wake up wait4 calls.
     */
    __wake_up_parent(tsk, parent);
    spin_unlock_irqrestore(&sighand->siglock, flags);
}

bool c_has_pending_signals(sigset_t* signal, sigset_t* blocked)
{
    unsigned long ready;
    long i;

    for (i = _NSIG_WORDS, ready = 0; --i >= 0;)
        ready |= signal->sig[i] & ~blocked->sig[i];

    return ready != 0;
}

bool c_recalc_sigpending_tsk(struct task_struct* t)
{
    if ((t->jobctl & (JOBCTL_PENDING_MASK | JOBCTL_TRAP_FREEZE))
        || PENDING(&t->pending, &t->blocked)
        || PENDING(&t->signal->shared_pending, &t->blocked)
        || cgroup_task_frozen(t))
    {
        set_tsk_thread_flag(t, TIF_SIGPENDING);
        return true;
    }

    /*
     * We must never clear the flag in another thread, or in current
     * when it's possible the current syscall is returning -ERESTART*.
     * So we don't clear it here, and only callers who know they should
     * do.
     */
    return false;
}

void c_ptrace_stop(task_t* task,
                   int exit_code,
                   int why,
                   int clear_code,
                   kernel_siginfo_t* info)
  __releases(&task->sighand->siglock) __acquires(&task->sighand->siglock)
{
    bool gstop_done = false;
    unsigned long flags;

    if (arch_ptrace_stop_needed(exit_code, info))
    {
        /*
         * The arch code has something special to do before a
         * ptrace stop.  This is allowed to block, e.g. for faults
         * on user stack pages.  We can't keep the siglock while
         * calling arch_ptrace_stop, so we must release it now.
         * To preserve proper semantics, we must do this before
         * any signal bookkeeping like checking group_stop_count.
         * Meanwhile, a SIGKILL could come in before we retake the
         * siglock.  That must prevent us from sleeping in TASK_TRACED.
         * So after regaining the lock, we must check for SIGKILL.
         */
        spin_unlock_irq(&task->sighand->siglock);
        arch_ptrace_stop(exit_code, info);
        spin_lock_irq(&task->sighand->siglock);
        if (c_sigkill_pending(task))
            return;
    }

    raw_spin_lock_irqsave(&task->pi_lock, flags);
    task->state = TASK_TRACED;
    raw_spin_unlock_irqrestore(&task->pi_lock, flags);
    /*
     * We're committing to trapping.  TRACED should be visible before
     * TRAPPING is cleared; otherwise, the tracer might fail do_wait().
     * Also, transition to TRACED and updates to ->jobctl should be
     * atomic with respect to siglock and should be done after the arch
     * hook as siglock is released and regrabbed across it.
     *
     *     TRACER				    TRACEE
     *
     *     ptrace_attach()
     * [L]   wait_on_bit(JOBCTL_TRAPPING)	[S] set_special_state(TRACED)
     *     do_wait()
     *       set_current_state()                smp_wmb();
     *       ptrace_do_wait()
     *         wait_task_stopped()
     *           task_stopped_code()
     * [L]         task_is_traced()		[S] task_clear_jobctl_trapping();
     */
    smp_wmb();

    task->last_siginfo = info;
    task->exit_code    = exit_code;

    /*
     * If @why is CLD_STOPPED, we're trapping to participate in a group
     * stop.  Do the bookkeeping.  Note that if SIGCONT was delievered
     * across siglock relocks since INTERRUPT was scheduled, PENDING
     * could be clear now.  We act as if SIGCONT is received after
     * TASK_TRACED is entered - ignore it.
     */
    if (why == CLD_STOPPED && (task->jobctl & JOBCTL_STOP_PENDING))
        gstop_done = c_task_participate_group_stop(task);

    /* any trap clears pending STOP trap, STOP trap clears NOTIFY */
    task_clear_jobctl_pending(task, JOBCTL_TRAP_STOP);

    if (info && info->si_code >> 8 == PTRACE_EVENT_STOP)
        task_clear_jobctl_pending(task, JOBCTL_TRAP_NOTIFY);

    /* entering a trap, clear TRAPPING */
    task_clear_jobctl_trapping(task);

    spin_unlock_irq(&task->sighand->siglock);
    read_lock(ptasklist_lock);

    if (c_may_ptrace_stop(task))
    {
        /*
         * Notify parents of the stop.
         *
         * While ptraced, there are two parents - the ptracer and
         * the real_parent of the group_leader.  The ptracer should
         * know about every stop while the real parent is only
         * interested in the completion of group stop.  The states
         * for the two don't interact with each other.  Notify
         * separately unless they're gonna be duplicates.
         */
        c_do_notify_parent_cldstop(task, true, why);
        if (gstop_done && ptrace_reparented(task))
            c_do_notify_parent_cldstop(task, false, why);

        /*
         * Don't want to allow preemption here, because
         * sys_ptrace() needs this task to be inactive.
         *
         * XXX: implement read_unlock_no_resched().
         */
        preempt_disable();
        read_unlock(ptasklist_lock);
        c_cgroup_enter_frozen(task);
        barrier();
        preempt_count_dec();
        freezable_schedule();
        c_cgroup_leave_frozen(task, true);
    }
    else
    {
        /*
         * By the time we got the lock, our tracer went away.
         * Don't drop the lock yet, another tracer may come.
         *
         * If @gstop_done, the ptracer went away between group stop
         * completion and here.  During detach, it would have set
         * JOBCTL_STOP_PENDING on us and we'll re-enter
         * TASK_STOPPED in do_signal_stop() on return, so notifying
         * the real parent of the group stop completion is enough.
         */
        if (gstop_done)
            c_do_notify_parent_cldstop(task, false, why);

        /* tasklist protects us from ptrace_freeze_traced() */
        task->state = TASK_RUNNING;
        if (clear_code)
            task->exit_code = 0;
        read_unlock(ptasklist_lock);
    }

    /*
     * We are back.  Now reacquire the siglock before touching
     * last_siginfo, so that we are sure to have synchronized with
     * any signal-sending on another CPU that wants to examine it.
     */
    spin_lock_irq(&task->sighand->siglock);
    task->last_siginfo = NULL;

    /* LISTENING can be set only during STOP traps, clear it */
    task->jobctl &= ~JOBCTL_LISTENING;

    /*
     * Queued signals ignored us while we were stopped for tracing.
     * So check for any that we should take before resuming user mode.
     * This sets TIF_SIGPENDING, but never clears it.
     */
    c_recalc_sigpending_tsk(task);
}

void c_ptrace_do_notify(task_t* task, int signr, int exit_code, int why)
{
    kernel_siginfo_t info;

    /**
     * We can say that the signal comes from our current process (kernel
     * thread or the remote task that asked to do the clone), it's not a
     * problem.
     */

    clear_siginfo(&info);
    info.si_signo = signr;
    info.si_code  = exit_code;
    info.si_pid   = task_pid_vnr(task);
    info.si_uid   = from_kuid_munged(current_user_ns(), current_uid());

    /* Let the debugger run.  */
    c_ptrace_stop(task, exit_code, why, 1, &info);
}

void c_ptrace_notify(task_t* task, int exit_code)
{
    BUG_ON((exit_code & (0x7f | ~0xffff)) != SIGTRAP);

    if (unlikely(task->task_works))
        task_work_run();

    spin_lock_irq(&task->sighand->siglock);
    c_ptrace_do_notify(task, SIGTRAP, exit_code, CLD_TRAPPED);
    spin_unlock_irq(&task->sighand->siglock);
}

void c_ptrace_event(task_t* task, int event, unsigned long message)
{
    if (unlikely(ptrace_event_enabled(task, event)))
    {
        task->ptrace_message = message;
        c_ptrace_notify(task, (event << 8) | SIGTRAP);
    }
    else if (event == PTRACE_EVENT_EXEC)
    {
        /* legacy EXEC report via SIGTRAP */
        if ((task->ptrace & (PT_PTRACED | PT_SEIZED)) == PT_PTRACED)
            send_sig(SIGTRAP, task, 0);
    }
}

void c_ptrace_event_pid(task_t* task, int event, struct pid* pid)
{
    unsigned long message = 0;
    struct pid_namespace* ns;

    rcu_read_lock();
    ns = task_active_pid_ns(rcu_dereference(task->parent));

    if (ns)
        message = pid_nr_ns(pid, ns);

    rcu_read_unlock();

    c_ptrace_event(task, event, message);
}
