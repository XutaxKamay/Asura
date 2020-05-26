#ifndef INCLUDES_AND_DEFS_H
#define INCLUDES_AND_DEFS_H

#include <asm/fpu/internal.h>
#include <asm/resctrl_sched.h>
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
#include <linux/nsproxy.h>
#include <linux/pagewalk.h>
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
#include <linux/writeback.h>
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
};

#endif
