#ifndef MAIN_H
#define MAIN_H
// clang-format off

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/syscalls.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/mman.h>
#include <linux/mempolicy.h>

#ifdef __PAGETABLE_P4D_FOLDED
#include <linux/sched/mm.h>
#endif

typedef void* ptr_t;

//#include <asm-generic/unistd.h>

#define c_printk( c_args, ... ) pr_cont( "[CUSTOM] "c_args, ##__VA_ARGS__ )

// clang-format on
#endif
