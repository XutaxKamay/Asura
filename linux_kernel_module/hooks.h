#ifndef HOOKS_H
#define HOOKS_H

#include "communicate.h"

typedef struct task_struct *(*copy_process_t)(struct pid *, int, int,
					      struct kernel_clone_args *);
typedef int (*exec_binprm_t)(struct linux_binprm *bprm);

/* Let's hook the kernel directly to know when a task_struct is inserted */

int find_copy_process(void);
int find_exec_binprm(void);
void hook_callsof_copy_process(void);
void hook_callsof_exec_binprm(void);
void unhook_callsof_copy_process(void);
void unhook_callsof_exec_binprm(void);
void hook_kernel(void);
void unhook_kernel(void);

extern copy_process_t original_copy_process;
extern exec_binprm_t original_exec_binprm;
extern struct buffer_struct buffer_list_calls_copy_process;
extern struct buffer_struct buffer_list_calls_exec_binprm;

#endif
