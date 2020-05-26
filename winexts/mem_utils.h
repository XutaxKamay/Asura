
#ifndef MEM_UTILS_H
#define MEM_UTILS_H

/**
 * 0xffffffff81000000 is the usual base address of text section.
 */
#define KERNEL_USUAL_TEXT_SECTION 0xffffffff81000000

typedef struct buffer_struct
{
    ptr_t addr;
    size_t size;
} buffer_t;

typedef struct temp_symbol
{
    const char* name;
    uintptr_t addr;
} temp_symbol_t;

/**
 * Buffers
 */
void alloc_buffer(size_t size, buffer_t* buffer);
int realloc_buffer(size_t size, buffer_t* buffer);
void free_buffer(buffer_t* buffer);
void init_buffer(buffer_t* buffer);

/* task_struct and mm_struct utils */
task_t* find_task_from_addr(uintptr_t special_address);
void vm_flags_to_string(vm_area_t* vma, char* output, int size);
int vm_flags_to_prot(vm_area_t* vma);
int prot_to_vm_flags(int prot);
mm_t* get_task_mm_kthread(task_t* task);
task_t* find_task_from_pid(pid_t pid);
int c_find_vma_from_task(task_t* task,
                         vm_area_t** vma_start,
                         uintptr_t wanted_addr);
int c_find_vma_from_task_str(task_t* task,
                             vm_area_t** vma_start,
                             const char* name);
void c_print_vmas(task_t* task);
int c_munmap(task_t* task, uintptr_t start);
void change_vm_flags(vm_area_t* vma, int new_flags, int* old_flags);
vm_area_t* c_mmap(task_t* task,
                  uintptr_t address,
                  uintptr_t size,
                  int prot);
int c___vm_munmap(task_t* task,
                  unsigned long start,
                  size_t len,
                  bool downgrade);
int c_vma_count(mm_t* mm);
int c_mprotect(task_t* task,
               uintptr_t start,
               uintptr_t size,
               int prot,
               int pkey);

/**
 * Copies memory from a task in its virtual address space to kernel one
 */
unsigned long c_copy_from_user(task_t* task,
                               ptr_t to,
                               ptr_t from,
                               size_t size);

/**
 * Copies memory to a task in its virtual address space to kernel one
 */
unsigned long c_copy_to_user(task_t* task,
                             ptr_t to,
                             ptr_t from,
                             size_t size);

/**
 * Memory offsets stuffs
 */
uintptr_t map_base_task(task_t* task);
uintptr_t kernel_offset(void);
uintptr_t align_address(uintptr_t address, size_t size);
ptr_t* find_sys_call_table(void);
uintptr_t find_in_system_map_symbol(const char* name);
uintptr_t c_find_sym_addr(const char* name);

/**
 * Paging stuffs for kernels that have RWX strict protection
 */
#ifndef __arch_um__
pte_t* get_pte(uintptr_t address);
pteval_t get_page_flags(uintptr_t addr);
pteval_t set_page_flags(uintptr_t addr, pteval_t val);
#endif

#endif
