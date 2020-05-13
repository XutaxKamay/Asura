
#ifndef MEMUTILS_H
#define MEMUTILS_H

typedef struct buffer_struct
{
    ptr_t addr;
    size_t size;
} buffer_t;

typedef struct pattern_result_struct
{
    uintptr_t* addrs;
    int count;
} pattern_result_t;

void init_pattern_result(pattern_result_t* pattern_result, size_t count);
bool realloc_pattern_result(pattern_result_t* pattern_result,
                            size_t add_count);
void free_pattern_result(pattern_result_t* pattern_result);
void vm_flags_to_string(vm_area_t* vma, char* output, int size);
int vm_flags_to_prot(vm_area_t* vma);
int prot_to_vm_flags(int prot);
void change_vm_flags(vm_area_t* vma, int new_flags, int* old_flags);
int c_find_vma_from_task(task_t* task,
                         vm_area_t** vma_start,
                         uintptr_t wanted_addr);
int c_find_vma_from_task_str(task_t* task,
                             vm_area_t** vma_start,
                             const char* name);
void c_print_vmas(task_t* task);
task_t* find_task_from_addr(uintptr_t address);
int scan_task(task_t* task,
              char* pattern,
              int len,
              pattern_result_t* pattern_result);
int scan_kernel(char* start,
                char* end,
                char* pattern,
                int len,
                pattern_result_t* pattern_result);
int scan_pattern(uintptr_t start,
                 uintptr_t end,
                 char* pattern,
                 int len,
                 pattern_result_t* pattern_result);
uintptr_t map_base_task(task_t* task);
uintptr_t kernel_offset(void);
uintptr_t align_address(uintptr_t address, size_t size);
ptr_t* find_sys_call_table(void);
void alloc_buffer(size_t size, buffer_t* buffer);
int realloc_buffer(size_t size, buffer_t* buffer);
void free_buffer(buffer_t* buffer);
void init_buffer(buffer_t* buffer);
unsigned long
c_copy_from_user(task_t* task, ptr_t to, ptr_t from, size_t size);
unsigned long
c_copy_to_user(task_t* task, ptr_t to, ptr_t from, size_t size);
mm_t* get_task_mm_kthread(task_t* task);
task_t* find_task_from_pid(pid_t pid);
int c_munmap(task_t* task, uintptr_t start);
void c_mmput(task_t*task, mm_t*mm);
task_t* debug_get_current(void);

/* Kernels that have RWX strict protection */
#ifndef __arch_um__
pte_t* get_pte(uintptr_t address);
pteval_t get_page_flags(uintptr_t addr);
pteval_t set_page_flags(uintptr_t addr, pteval_t val);
#else
void set_current(struct task_struct* task);
#endif

#endif
