#ifndef COMMUNICATE_H
#define COMMUNICATE_H

#include "memutils.h"
#include "communicate_structs.h"
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/mutex.h>

#define PROT_COMMUNICATE      PROT_READ | PROT_WRITE
#define MAGIC_ADDRESS         0x13370000
#define MAX_COMMANDS          0x100
#define COMMUNCIATE_ZERO_CMDS 0

extern struct mutex g_task_communicate_mutex;
extern struct task_struct* g_task_communicate;
extern bool g_task_communicate_stop;

struct communicate_header_struct
communicate_read_header(struct task_struct* task, ptr_t address);
void communicate_reset(struct task_struct* task, ptr_t address);
void communicate_check_task(struct task_struct* task);
void communicate_check_tasks(void);
void communicate_with_task(struct task_struct* task);
void communicate_with_tasks(void);
void communicate_free_tasks(void);
void communicate_thread_with_tasks(bool only_once);
void communicate_start_thread(bool only_once);
void communicate_kill_thread(void);

#endif
