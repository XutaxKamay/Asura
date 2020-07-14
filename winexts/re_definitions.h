#ifndef RE_DEFINITIONS_H
#define RE_DEFINITIONS_H

/**
 * Functions/symbols that the kernel doesn't export by default which is
 * needed by the kernel module
 */

/**
 * This one is found in memory instead
 */

extern struct tracepoint* __tracepoint_sched_process_fork_ptr;

int find_all_symbols(void);

void vma_rb_erase(vm_area_t* vma, rb_root_t* root);
struct task_struct* copy_process(struct pid* pid,
                                 int trace,
                                 int node,
                                 struct kernel_clone_args* args);

void switch_to_extra(struct task_struct* prev, struct task_struct* next);

/*
 * Like page_order(), but for callers who cannot afford to hold the zone
 * lock. PageBuddy() should be checked first by the caller to minimize
 * race window, and invalid values must be handled gracefully.
 *
 * READ_ONCE is used so that if the caller assigns the result into a local
 * variable and e.g. tests it for valid range before using, the compiler
 * cannot decide to remove the variable and inline the page_private(page)
 * multiple times, potentially observing different values in the tests and
 * the actual use of the result.
 */
#define page_order_unsafe(page) READ_ONCE(page_private(page))

static inline bool is_cow_mapping(vm_flags_t flags)
{
    return (flags & (VM_SHARED | VM_MAYWRITE)) == VM_MAYWRITE;
}

/*
 * These three helpers classifies VMAs for virtual memory accounting.
 */

/*
 * Executable code area - executable, not writable, not stack
 */
static inline bool is_exec_mapping(vm_flags_t flags)
{
    return (flags & (VM_EXEC | VM_WRITE | VM_STACK)) == VM_EXEC;
}

/*
 * Stack area - atomatically grows in one direction
 *
 * VM_GROWSUP / VM_GROWSDOWN VMAs are always private anonymous:
 * do_mmap() forbids all other combinations.
 */
static inline bool is_stack_mapping(vm_flags_t flags)
{
    return (flags & VM_STACK) == VM_STACK;
}

/*
 * Data area - private, writable, not stack
 */
static inline bool is_data_mapping(vm_flags_t flags)
{
    return (flags & (VM_WRITE | VM_SHARED | VM_STACK)) == VM_WRITE;
}

int copy_namespaces(unsigned long flags, struct task_struct* tsk);

void validate_mm(struct mm_struct* mm);

long populate_vma_page_range(struct vm_area_struct* vma,
                             unsigned long start,
                             unsigned long end,
                             int* nonblocking);

#endif
