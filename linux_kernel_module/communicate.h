#ifndef COMMUNICATE_H
#define COMMUNICATE_H

#include "memutils.h"
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/delay.h>

#define PROT_RWX PROT_READ | PROT_WRITE | PROT_EXEC
#define MAGIC_ADDRESS 0x13370000

void communicate_start_thread(void);
void communicate_kill_thread(void);
void communicate_alloc_vmas(void);
void hook_syscalls(void);
void unhook_syscalls(void);

extern struct task_struct *g_task_communicate;

#endif
