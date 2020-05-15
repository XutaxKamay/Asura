#ifndef INCLUDES_AND_DEFS
#define INCLUDES_AND_DEFS

#include <asm/switch_to.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/errno.h>
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
#include <linux/sched/debug.h>
#include <linux/sched/task.h>
#include <linux/sched/task_stack.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/types.h>
#include <linux/userfaultfd_k.h>
#include <linux/version.h>

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

#endif
