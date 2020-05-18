#ifndef INCLUDES_AND_DEFS_H
#define INCLUDES_AND_DEFS_H

#include <asm/switch_to.h>
#include <linux/anon_inodes.h>
#include <linux/audit.h>
#include <linux/cdev.h>
#include <linux/cgroup.h>
#include <linux/cn_proc.h>
#include <linux/cpu.h>
#include <linux/delayacct.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/fdtable.h>
#include <linux/freezer.h>
#include <linux/fs.h>
#include <linux/fs_struct.h>
#include <linux/futex.h>
#include <linux/gpio.h>
#include <linux/hugetlb.h>
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/khugepaged.h>
#include <linux/ksm.h>
#include <linux/livepatch.h>
#include <linux/mempolicy.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/mmu_context.h>
#include <linux/mmu_notifier.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/profile.h>
#include <linux/ptrace.h>
#include <linux/random.h>
#include <linux/regset.h>
#include <linux/sched/autogroup.h>
#include <linux/sched/cputime.h>
#include <linux/sched/debug.h>
#include <linux/sched/task.h>
#include <linux/sched/task_stack.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/stackleak.h>
#include <linux/syscalls.h>
#include <linux/task_io_accounting_ops.h>
#include <linux/task_work.h>
#include <linux/taskstats_kern.h>
#include <linux/topology.h>
#include <linux/trace.h>
#include <linux/tracehook.h>
#include <linux/tsacct_kern.h>
#include <linux/tty.h>
#include <linux/types.h>
#include <linux/user-return-notifier.h>
#include <linux/userfaultfd_k.h>
#include <linux/utsname.h>
#include <linux/version.h>
#include <linux/vmacache.h>
#include <trace/events/sched.h>

#ifdef __PAGETABLE_P4D_FOLDED
    #include <linux/sched/mm.h>
#endif

#define DEVICE_FILE_NAME  "winexts_dev"
#define DEVICE_FMT        "winexts"
#define DEVICE_CLASS_NAME "winexts_class"

#define c_printk(c_args, ...)                                            \
    printk(KERN_INFO "[winexts] " c_args, ##__VA_ARGS__)
#define c_printk_error(c_args, ...)                                      \
    printk(KERN_ERR "[winexts] " c_args, ##__VA_ARGS__)
#define c_printk_warn(c_args, ...)                                       \
    printk(KERN_WARNING "[winexts] " c_args, ##__VA_ARGS__)
#define c_printk_info(c_args, ...)                                       \
    printk(KERN_INFO "[winexts] " c_args, ##__VA_ARGS__)

typedef void* ptr_t;
typedef ptr_t* pptr_t;

typedef unsigned char byte_t;
typedef byte_t* bytes_t;

typedef struct task_struct task_t;
typedef struct mm_struct mm_t;
typedef struct page page_t;
typedef struct vm_area_struct vm_area_t;
typedef struct rb_root rb_root_t;

#ifdef __arch_um__
typedef struct cpu_task
{
    int pid;
    void* task;
} cpu_task_t;
#endif

/*
 * This is the main, per-CPU runqueue data structure.
 * This data should only be modified by the local cpu.
 */
struct rq
{
    raw_spinlock_t* lock;
    raw_spinlock_t* orig_lock;

    struct task_struct __rcu* curr;
    struct task_struct* idle;
    struct task_struct* stop;
    struct mm_struct* prev_mm;

    unsigned int nr_running;
    /*
     * This is part of a global counter where only the total sum
     * over all CPUs matters. A task can increase this counter on
     * one CPU and if it got migrated afterwards it may decrease
     * it on another CPU. Always updated under the runqueue lock:
     */
    unsigned long nr_uninterruptible;
    u64 nr_switches;

    /* Stored data about rq->curr to work outside rq lock */
    u64 rq_deadline;
    int rq_prio;

    /* Best queued id for use outside lock */
    u64 best_key;

    unsigned long last_scheduler_tick; /* Last jiffy this RQ ticked */
    unsigned long last_jiffy; /* Last jiffy this RQ updated rq clock */
    u64 niffies;              /* Last time this RQ updated rq clock */
    u64 last_niffy;           /* Last niffies as updated by local clock */
    u64 last_jiffy_niffies;   /* Niffies @ last_jiffy */

    u64 load_update;        /* When we last updated load */
    unsigned long load_avg; /* Rolling load average */
#ifdef CONFIG_HAVE_SCHED_AVG_IRQ
    u64 irq_load_update;        /* When we last updated IRQ load */
    unsigned long irq_load_avg; /* Rolling IRQ load average */
#endif
#ifdef CONFIG_SMT_NICE
    struct mm_struct* rq_mm;
    int rq_smt_bias; /* Policy/nice level bias across smt siblings */
#endif
    /* Accurate timekeeping data */
    unsigned long user_ns, nice_ns, irq_ns, softirq_ns, system_ns,
      iowait_ns, idle_ns;
    atomic_t nr_iowait;

#ifdef CONFIG_MEMBARRIER
    int membarrier_state;
#endif

    skiplist_node* node;
    skiplist* sl;
#ifdef CONFIG_SMP
    struct task_struct* preempt;    /* Preempt triggered on this task */
    struct task_struct* preempting; /* Hint only, what task is preempting
                                     */

    int cpu; /* cpu of this runqueue */
    bool online;

    struct root_domain* rd;
    struct sched_domain* sd;

    unsigned long cpu_capacity_orig;

    int* cpu_locality;    /* CPU relative cache distance */
    struct rq** rq_order; /* Shared RQs ordered by relative cache distance
                           */
    struct rq** cpu_order; /* RQs of discrete CPUs ordered by distance */

    bool is_leader;
    struct rq* smp_leader; /* First physical CPU per node */
    #ifdef CONFIG_SCHED_SMT
    struct rq* smt_leader; /* First logical CPU in SMT siblings */
    cpumask_t thread_mask;
    bool (*siblings_idle)(struct rq* rq);
    /* See if all smt siblings are idle */
    #endif /* CONFIG_SCHED_SMT */
    #ifdef CONFIG_SCHED_MC
    struct rq* mc_leader; /* First logical CPU in MC siblings */
    cpumask_t core_mask;
    bool (*cache_idle)(struct rq* rq);
    /* See if all cache siblings are idle */
    #endif /* CONFIG_SCHED_MC */
#endif /* CONFIG_SMP */
#ifdef CONFIG_IRQ_TIME_ACCOUNTING
    u64 prev_irq_time;
#endif /* CONFIG_IRQ_TIME_ACCOUNTING */
#ifdef CONFIG_PARAVIRT
    u64 prev_steal_time;
#endif /* CONFIG_PARAVIRT */
#ifdef CONFIG_PARAVIRT_TIME_ACCOUNTING
    u64 prev_steal_time_rq;
#endif /* CONFIG_PARAVIRT_TIME_ACCOUNTING */

    u64 clock, old_clock, last_tick;
    /* Ensure that all clocks are in the same cache line */
    u64 clock_task ____cacheline_aligned;
    int dither;

    int iso_ticks;
    bool iso_refractory;

#ifdef CONFIG_HIGH_RES_TIMERS
    struct hrtimer hrexpiry_timer;
#endif

    int rt_nr_running; /* Number real time tasks running */
#ifdef CONFIG_SCHEDSTATS

    /* latency stats */
    struct sched_info rq_sched_info;
    unsigned long long rq_cpu_time;
    /* could above be rq->cfs_rq.exec_clock + rq->rt_rq.rt_runtime ? */

    /* sys_sched_yield() stats */
    unsigned int yld_count;

    /* schedule() stats */
    unsigned int sched_switch;
    unsigned int sched_count;
    unsigned int sched_goidle;

    /* try_to_wake_up() stats */
    unsigned int ttwu_count;
    unsigned int ttwu_local;
#endif /* CONFIG_SCHEDSTATS */

#ifdef CONFIG_SMP
    struct llist_head wake_list;
#endif

#ifdef CONFIG_CPU_IDLE
    /* Must be inspected within a rcu lock section */
    struct cpuidle_state* idle_state;
#endif
};

#endif
