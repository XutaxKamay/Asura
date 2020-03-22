#ifndef COMMUNICATE_H
#define COMMUNICATE_H

#include "memutils.h"
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/delay.h>

#define PROT_RWX PROT_READ | PROT_WRITE | PROT_EXEC
#define MAGIC_ADDRESS 0x13370000
#define BACKUP_HOOK_BYTES 16

void communicate_start_thread(void);
void communicate_kill_thread(void);
void communicate_alloc_vmas(void);
/* Let's hook the kernel directly to know when a task_struct is inserted */
void hook_kernel(void);
void unhook_kernel(void);
void hook_callsof_copy_process(void);
void unhook_callsof_copy_process(void);

typedef struct task_struct *(*copy_process_t)(struct pid *, int, int,
					      struct kernel_clone_args *);
extern struct task_struct *g_task_communicate;

#endif
