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
			 uintptr_t wanted_addr);
int c_find_vma_from_task_str(struct task_struct *task,
			     struct vm_area_struct **vma_start,
			     const char *name);
void c_print_vmas(struct task_struct *task);
struct task_struct *find_task_from_addr(uintptr_t address);
int scan_task(struct task_struct *task, char *pattern, int len,
	      struct buffer_struct *buf);
int scan_kernel(char *start, char *end, char *pattern, int len,
		struct buffer_struct *buf);
int scan_pattern(uintptr_t start, uintptr_t end, char *pattern, int len,
		 struct buffer_struct *buf);
uintptr_t map_base_task(struct task_struct *task);
uintptr_t kernel_offset(void);
int remote_mprotect(struct task_struct *task, uintptr_t address, int new_flags,
		    int *old_flags);
struct vm_area_struct *remote_mmap(struct task_struct *task, uintptr_t address,
				   int prot);
uintptr_t align_address(uintptr_t address, size_t size);
ptr_t *find_sys_call_table(void);
void alloc_buffer(size_t size, struct buffer_struct *buffer);
int realloc_buffer(size_t size, struct buffer_struct *buffer);
void free_buffer(struct buffer_struct *buffer);
void init_buffer(struct buffer_struct *buffer);
unsigned long c_copy_from_user(struct task_struct *task, ptr_t to, ptr_t from,
			       size_t size);
unsigned long c_copy_to_user(struct task_struct *task, ptr_t to, ptr_t from,
			     size_t size);
struct mm_struct *get_task_mm_kthread(struct task_struct *task);

/* Kernels that have RWX strict protection */
#ifndef __arch_um__
pte_t *get_pte(uintptr_t address);
#endif

#endif
