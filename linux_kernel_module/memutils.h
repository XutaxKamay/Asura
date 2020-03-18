#ifndef KERNEL_CUSTOM_MEMUTILS
#define KERNEL_CUSTOM_MEMUTILS

#include "main.h"
#include "utils.h"

struct buffer_struct {
	void *addr;
	size_t size;
};

void vm_flags_to_string(struct vm_area_struct *vma, char *output, int size);
int vm_flags_to_prot(struct vm_area_struct *vma);
int prot_to_vm_flags(int prot);
void change_vm_flags(struct vm_area_struct *vma, int new_flags, int *old_flags);
int c_find_vma_from_task(struct task_struct *task,
			 struct vm_area_struct **vma_start,
			 unsigned long wanted_addr);
int c_find_vma_from_task_str(struct task_struct *task,
			     struct vm_area_struct **vma_start,
			     const char *name);
void c_print_vmas(struct task_struct *task);
struct task_struct *find_task_from_addr(unsigned long address);
int scan_task(struct task_struct *task, char *pattern, int len,
	      struct buffer_struct *buf);
int scan_kernel(char *start, char *end, char *pattern, int len,
		struct buffer_struct *buf);
int scan_pattern(unsigned long start, unsigned long end, char *pattern, int len,
		 struct buffer_struct *buf);
unsigned long map_base_task(struct task_struct *task);
unsigned long kernel_offset(void);
int remote_mprotect(pid_t pid, uintptr_t address, int new_flags,
		    int *old_flags);
struct vm_area_struct *remote_mmap(pid_t pid, uintptr_t address, int prot);
uintptr_t align_address(uintptr_t address, size_t size);
#ifndef __arch_um__
pte_t *get_pte(uintptr_t address);
#endif
ptr_t *find_sys_call_table(void);
void alloc_buffer(size_t size, struct buffer_struct *buffer);
void realloc_buffer(size_t size, struct buffer_struct *buffer);
void free_buffer(struct buffer_struct *buffer);
void init_buffer(struct buffer_struct *buffer);

#endif
