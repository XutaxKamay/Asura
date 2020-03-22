#ifndef COMMUNICATE_H
#define COMMUNICATE_H

#include "memutils.h"
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/delay.h>

#define PROT_COMMUNICATE PROT_READ | PROT_WRITE
#define MAGIC_ADDRESS 0x13370000
#define MAX_COMMANDS 0x100
#define COMMUNCIATE_ZERO_CMDS 0

enum communicate_cmd {
	COMMUNICATE_READ,
	COMMUNICATE_WRITE,
	COMMUNCIATE_THREAD,
	COMMUNICATE_LIST_VMA
};

struct communicate_write_struct {
	uintptr_t vm_local_address;
	uintptr_t vm_remote_address;
	uintptr_t vm_size;
};

struct communicate_read_struct {
	uintptr_t vm_local_address;
	uintptr_t vm_remote_address;
	uintptr_t vm_size;
};

void communicate_start_thread(void);
void communicate_kill_thread(void);
void communicate_with_tasks(void);
/* Let's hook the kernel directly to know when a task_struct is inserted */
void hook_kernel(void);
void unhook_kernel(void);
void hook_callsof_copy_process(void);
void unhook_callsof_copy_process(void);

typedef struct task_struct *(*copy_process_t)(struct pid *, int, int,
					      struct kernel_clone_args *);
extern struct task_struct *g_task_communicate;

#endif
