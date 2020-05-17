#ifndef INCLUDES_AND_DEFS_H
#define INCLUDES_AND_DEFS_H

#include <asm/switch_to.h>
#include <linux/cdev.h>
#include <linux/cgroup.h>
#include <linux/cpu.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/freezer.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/mempolicy.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/mmu_context.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/profile.h>
#include <linux/ptrace.h>
#include <linux/random.h>
#include <linux/regset.h>
#include <linux/sched/cputime.h>
#include <linux/sched/debug.h>
#include <linux/sched/task.h>
#include <linux/sched/task_stack.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/syscalls.h>
#include <linux/task_work.h>
#include <linux/trace.h>
#include <linux/tracehook.h>
#include <linux/types.h>
#include <linux/userfaultfd_k.h>
#include <linux/utsname.h>
#include <linux/version.h>
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

#define cpu_rq(cpu)   (&per_cpu(runqueues, (cpu)))
#define this_rq()     this_cpu_ptr(&runqueues)
#define task_rq(p)    cpu_rq(task_cpu(p))
#define cpu_curr(cpu) (cpu_rq(cpu)->curr)
#define raw_rq()      raw_cpu_ptr(&runqueues)

/*
 * Like page_order(), but for callers who cannot afford to hold the zone
 * lock. PageBuddy() should be checked first by the caller to minimize
 * race window, and invalid values must be handled gracefully.
 *
 * READ_ONCE is used so that if the caller assigns the result into a local
 * variable and e.g. tests it for valid range before using, the compiler
 * cannot decide to remove the variable and inline the page_private(page)
 * multiple times, potentially observing different values in the tests and
 * the actual use of the result.
 */
#define page_order_unsafe(page) READ_ONCE(page_private(page))

static inline bool is_cow_mapping(vm_flags_t flags)
{
    return (flags & (VM_SHARED | VM_MAYWRITE)) == VM_MAYWRITE;
}

/*
 * These three helpers classifies VMAs for virtual memory accounting.
 */

/*
 * Executable code area - executable, not writable, not stack
 */
static inline bool is_exec_mapping(vm_flags_t flags)
{
    return (flags & (VM_EXEC | VM_WRITE | VM_STACK)) == VM_EXEC;
}

/*
 * Stack area - atomatically grows in one direction
 *
 * VM_GROWSUP / VM_GROWSDOWN VMAs are always private anonymous:
 * do_mmap() forbids all other combinations.
 */
static inline bool is_stack_mapping(vm_flags_t flags)
{
    return (flags & VM_STACK) == VM_STACK;
}

/*
 * Data area - private, writable, not stack
 */
static inline bool is_data_mapping(vm_flags_t flags)
{
    return (flags & (VM_WRITE | VM_SHARED | VM_STACK)) == VM_WRITE;
}

#endif
