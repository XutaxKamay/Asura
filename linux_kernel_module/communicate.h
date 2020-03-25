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
	COMMUNICATE_CMD_READ,
	COMMUNICATE_CMD_WRITE,
	COMMUNCIATE_CMD_CREATE_THREAD,
	COMMUNICATE_CMD_LIST_VMA
};

enum communicate_state {
	COMMUNICATE_STATE_STOPPED, // Communication is stopped.
	COMMUNICATE_STATE_READ_HEADER_ERROR, // Has header error.
	COMMUNICATE_STATE_INMODULE, // The commands are being processed by the module.
	COMMUNICATE_STATE_INPROCESS, // Commands are processed by the process.
	COMMUNICATE_STATE_MAX
};

enum communicate_read_error {
	COMMUNICATE_READ_ERROR_NO,
	COMMUNICATE_READ_ERROR_MAX
};

enum communicate_write_error {
	COMMUNICATE_WRITE_ERROR_NO,
	COMMUNICATE_WRITE_ERROR_MAX
};

struct communicate_header_struct {
	enum communicate_state state;
	int number_of_cmds;
	ptr_t *cmds;
};

struct communicate_write_struct {
	uintptr_t vm_local_address;
	uintptr_t vm_remote_address;
	uintptr_t vm_size;
	pid_t pid_target;
	enum communicate_write_error has_error; // Only used by the module
};

struct communicate_read_struct {
	uintptr_t vm_local_address;
	uintptr_t vm_remote_address;
	uintptr_t vm_size;
	pid_t pid_target;
	enum communicate_read_error error; // Only used by the module
};

struct communicate_header_struct
communicate_read_header(struct task_struct *task, struct vm_area_struct *vma);
void communicate_reset(struct task_struct *task, struct vm_area_struct *vma);
void communicate_check_task(struct task_struct *task);
void communicate_check_tasks(void);
void communicate_with_task(struct task_struct *task);
void communicate_with_tasks(void);
void communicate_thread_with_tasks(bool only_once);
void communicate_start_thread(bool only_once);
void communicate_kill_thread(void);

#endif
