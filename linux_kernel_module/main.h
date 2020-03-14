#ifndef MAIN_H
#define MAIN_H
// clang-format off

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/syscalls.h>
#include <linux/mm.h>
#include <linux/hugetlb.h>
#include <linux/shm.h>
#include <linux/mman.h>
#include <linux/fs.h>
#include <linux/highmem.h>
#include <linux/security.h>
#include <linux/mempolicy.h>
#include <linux/personality.h>
#include <linux/swap.h>
#include <linux/swapops.h>
#include <linux/mmu_notifier.h>
#include <linux/migrate.h>
#include <linux/perf_event.h>
#include <linux/pkeys.h>
#include <linux/ksm.h>
#include <linux/uaccess.h>
#include <linux/mm_inline.h>
#include <linux/fs_struct.h>
#include <linux/kthread.h>
#include <linux/version.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/ftrace.h>

#include <trace/syscall.h>
#include <trace/events/syscalls.h>

#include <asm/pgtable.h>
#include <asm/cacheflush.h>
#include <asm/mmu_context.h>
#include <asm/tlbflush.h>
#include <asm/syscalls.h>
#include <asm/syscall.h>
#include <asm/pgtable_types.h>


#ifdef __PAGETABLE_P4D_FOLDED
#include <linux/sched/mm.h>
#endif
#include <linux/elf.h>

typedef void* ptr_t;

//#include <asm-generic/unistd.h>

#define c_printk( c_args, ... ) pr_cont( "[CUSTOM] "c_args, ##__VA_ARGS__ )

// clang-format on
#endif
