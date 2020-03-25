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

extern struct task_struct *g_task_communicate;

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
	pid_t pid_target;
};

struct communicate_read_struct {
	uintptr_t vm_local_address;
	uintptr_t vm_remote_address;
	uintptr_t vm_size;
	pid_t pid_target;
};

void communicate_check_task(struct task_struct *task);
void communicate_check_tasks(void);
void communicate_reset(struct task_struct *task, struct vm_area_struct *vma);
void communicate_with_task(struct task_struct *task);
void communicate_with_tasks(void);
void communicate_thread_with_tasks(bool only_once);
void communicate_start_thread(bool only_once);
void communicate_kill_thread(void);

#endif
