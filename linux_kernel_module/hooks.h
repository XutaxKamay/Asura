#ifndef HOOKS_H
#define HOOKS_H

#include "communicate.h"

typedef struct task_struct* (*copy_process_t)(struct pid*,
                                              int,
                                              int,
                                              struct kernel_clone_args*);
typedef int (*free_bprm_t)(struct linux_binprm* bprm);

/*
 * Let's hook the kernel directly to know when a task_struct is inserted
 */

int find_copy_process(void);
int find_free_bprm(void);

void hook_callsof_copy_process(void);
void hook_callsof_free_bprm(void);

void unhook_callsof_copy_process(void);
void unhook_callsof_free_bprm(void);

void hook_kernel(void);
void unhook_kernel(void);

extern copy_process_t original_copy_process;
extern free_bprm_t original_free_bprm;

#endif
