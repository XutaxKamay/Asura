#include "main.h"

/**
 * Functions/symbols that the kernel doesn't export by default which is
 * needed by the kernel module
 */

spinlock_t* pcss_set_lock = NULL;
rwlock_t* ptasklist_lock  = NULL;

int find_css_set_lock(void)
{
    pcss_set_lock = (spinlock_t*)(CSS_SET_LOCK_ADDR + kernel_offset());

    return 0;
}

int find_tasklist_lock(void)
{
    ptasklist_lock = (rwlock_t*)(TASKLIST_LOCK_ADDR + kernel_offset());

    return 0;
}

#ifdef __arch_um__
void set_current(struct task_struct* task)
{
    static struct cpu_task* cpu_tasks = NULL;
    static int* __userspace_pid       = NULL;

    if (cpu_tasks == NULL)
    {
        cpu_tasks = (struct cpu_task*)kallsyms_lookup_name("cpu_tasks");
    }

    if (__userspace_pid == NULL)
    {
        __userspace_pid = (int*)kallsyms_lookup_name("userspace_pid");
    }

    cpu_tasks[task_thread_info(task)->cpu] = ((
      struct cpu_task) { __userspace_pid[0], task });
}
#endif

void vma_rb_erase(vm_area_t* vma, rb_root_t* root)
{
    typedef void (*vma_rb_erase_t)(vm_area_t*, rb_root_t*);
    static vma_rb_erase_t p_vma_rb_erase = NULL;

    if (p_vma_rb_erase == NULL)
    {
        p_vma_rb_erase = (vma_rb_erase_t)kallsyms_lookup_name("__vma_rb_"
                                                              "erase");
    }

    if (p_vma_rb_erase == NULL)
    {
        c_printk_error("couldn't find vma_rb_erase\n");
        return;
    }

    return p_vma_rb_erase(vma, root);
}

/* Redefine functions */
vm_area_t* vm_area_alloc(mm_t* mm)
{
    typedef vm_area_t* (*vm_area_alloc_t)(mm_t*);
    static vm_area_alloc_t p_vm_area_alloc = NULL;

    if (p_vm_area_alloc == NULL)
    {
        p_vm_area_alloc = (vm_area_alloc_t)kallsyms_lookup_name("vm_area_"
                                                                "alloc");
    }

    if (p_vm_area_alloc == NULL)
    {
        c_printk_error("couldn't find vm_area_alloc\n");
        return NULL;
    }

    return p_vm_area_alloc(mm);
}

void vm_area_free(vm_area_t* vma)
{
    typedef void (*vm_area_free_t)(vm_area_t*);
    static vm_area_free_t p_vm_area_free = NULL;

    if (p_vm_area_free == NULL)
    {
        p_vm_area_free = (vm_area_free_t)kallsyms_lookup_name("vm_area_"
                                                              "free");
    }

    if (p_vm_area_free == NULL)
    {
        c_printk_error("couldn't find vm_area_free\n");
        return;
    }

    p_vm_area_free(vma);
}

int insert_vm_struct(mm_t* mm, vm_area_t* vma)
{
    typedef int (*insert_vm_struct_t)(mm_t*, vm_area_t*);

    static insert_vm_struct_t p_insert_vm_struct = NULL;

    if (p_insert_vm_struct == NULL)
    {
        p_insert_vm_struct = (insert_vm_struct_t)kallsyms_lookup_name(
          "insert_vm_struct");
    }

    if (p_insert_vm_struct == NULL)
    {
        c_printk_error("couldn't find insert_vm_struct\n");
        return -1;
    }

    return p_insert_vm_struct(mm, vma);
}

unsigned long ksys_mmap_pgoff(unsigned long addr,
                              unsigned long len,
                              unsigned long prot,
                              unsigned long flags,
                              unsigned long fd,
                              unsigned long pgoff)
{
    typedef unsigned long (*ksys_mmap_pgoff_t)(unsigned long addr,
                                               unsigned long len,
                                               unsigned long prot,
                                               unsigned long flags,
                                               unsigned long fd,
                                               unsigned long pgoff);

    static ksys_mmap_pgoff_t p_ksys_mmap_pgoff = NULL;

    if (p_ksys_mmap_pgoff == NULL)
    {
        p_ksys_mmap_pgoff = (ksys_mmap_pgoff_t)kallsyms_lookup_name(
          "ksys_mmap_pgoff");
    }

    if (p_ksys_mmap_pgoff == NULL)
    {
        c_printk_error("couldn't find ksys_mmap_pgoff\n");
        return -1;
    }

    return p_ksys_mmap_pgoff(addr, len, prot, flags, fd, pgoff);
}

long _do_fork(struct kernel_clone_args* kargs)
{
    typedef long (*_do_fork_t)(struct kernel_clone_args * kargs);
    static _do_fork_t p__do_fork = NULL;

    if (p__do_fork == NULL)
    {
        p__do_fork = (_do_fork_t)kallsyms_lookup_name("_do_fork");
    }

    if (p__do_fork == NULL)
    {
        c_printk_error("couldn't find _do_fork\n");
        return -1;
    }

    return p__do_fork(kargs);
}

struct task_struct* __switch_to(struct task_struct* prev,
                                struct task_struct* next)
{
    typedef struct task_struct* (
      *__switch_to_t)(struct task_struct * prev,
                      struct task_struct * next);
    static __switch_to_t p___switch_to = NULL;

    if (p___switch_to == NULL)
    {
        p___switch_to = (__switch_to_t)kallsyms_lookup_name("__switch_"
                                                            "to");
    }

    if (p___switch_to == NULL)
    {
        c_printk_error("couldn't find __switch_to\n");
        return NULL;
    }

    return p___switch_to(prev, next);
}

struct task_struct* __switch_to_asm(struct task_struct* prev,
                                    struct task_struct* next)
{
    typedef struct task_struct* (
      *__switch_to_asm_t)(struct task_struct * prev,
                          struct task_struct * next);
    static __switch_to_asm_t p___switch_to_asm = NULL;

    if (p___switch_to_asm == NULL)
    {
        p___switch_to_asm = (__switch_to_asm_t)kallsyms_lookup_name(
          "__switch_to_asm");
    }

    if (p___switch_to_asm == NULL)
    {
        c_printk_error("couldn't find __switch_to_asm\n");
        return NULL;
    }

    return p___switch_to_asm(prev, next);
}

int __do_munmap(struct mm_struct* mm,
                unsigned long start,
                size_t len,
                struct list_head* uf,
                bool downgrade)
{
    typedef int (*__do_munmap_t)(struct mm_struct * mm,
                                 unsigned long start,
                                 size_t len,
                                 struct list_head* uf,
                                 bool downgrade);

    static __do_munmap_t p___do_munmap = NULL;

    if (p___do_munmap == NULL)
    {
        p___do_munmap = (__do_munmap_t)kallsyms_lookup_name("__do_"
                                                            "munmap");
    }

    if (p___do_munmap == NULL)
    {
        c_printk_error("couldn't find __do_munmap\n");
        return -1;
    }

    return p___do_munmap(mm, start, len, uf, downgrade);
}

void set_task_cpu(struct task_struct* p, unsigned int cpu)
{
    typedef void (*set_task_cpu_t)(struct task_struct * p,
                                   unsigned int cpu);

    static set_task_cpu_t p_set_task_cpu = NULL;

    if (p_set_task_cpu == NULL)
    {
        p_set_task_cpu = (set_task_cpu_t)kallsyms_lookup_name("set_task_"
                                                              "cpu");
    }

    if (p_set_task_cpu == NULL)
    {
        c_printk_error("couldn't find set_task_cpu\n");
        return;
    }

    set_task_cpu(p, cpu);
}

void* vmalloc_exec(unsigned long size)
{
    typedef void* (*vmalloc_exec_t)(unsigned long size);

    static vmalloc_exec_t p_vmalloc_exec = NULL;

    if (p_vmalloc_exec == NULL)
    {
        p_vmalloc_exec = (vmalloc_exec_t)kallsyms_lookup_name("vmalloc_"
                                                              "exec");
    }

    if (p_vmalloc_exec == NULL)
    {
        c_printk_error("couldn't find vmalloc_exec\n");
        return NULL;
    }

    return p_vmalloc_exec(size);
}

struct task_struct* copy_process(struct pid* pid,
                                 int trace,
                                 int node,
                                 struct kernel_clone_args* args)
{
    typedef struct task_struct* (
      *copy_process_t)(struct pid*, int, int, struct kernel_clone_args*);

    static copy_process_t copy_process_ptr = NULL;

    if (copy_process_ptr == NULL)
    {
        copy_process_ptr = (copy_process_t)kallsyms_lookup_name(
          "copy_process");
    }

    if (copy_process_ptr == NULL)
    {
        c_printk_error("couldn't find copy_process\n");
        return NULL;
    }

    return copy_process_ptr(pid, trace, node, args);
}

void wake_up_new_task(struct task_struct* tsk)
{
    typedef void (*wake_up_new_task_t)(struct task_struct*);

    static wake_up_new_task_t p_wake_up_new_task = NULL;

    if (p_wake_up_new_task == NULL)
    {
        p_wake_up_new_task = (wake_up_new_task_t)kallsyms_lookup_name(
          "wake_up_new_task");
    }

    if (p_wake_up_new_task == NULL)
    {
        c_printk_error("couldn't find wake_up_new_task\n");
        return;
    }

    p_wake_up_new_task(tsk);
}

void ptrace_notify(int exit_code)
{
    typedef void (*ptrace_notify_t)(int);

    static ptrace_notify_t p_ptrace_notify = NULL;

    if (p_ptrace_notify == NULL)
    {
        p_ptrace_notify = (ptrace_notify_t)kallsyms_lookup_name("ptrace_"
                                                                "notify");
    }

    if (p_ptrace_notify == NULL)
    {
        c_printk_error("couldn't find ptrace_notify\n");
        return;
    }

    p_ptrace_notify(exit_code);
}

void cgroup_update_frozen(struct cgroup* cgrp)
{
    typedef void (*cgroup_update_frozen_t)(void*);
    static cgroup_update_frozen_t cgroup_update_frozen_ptr = NULL;

    if (cgroup_update_frozen_ptr == NULL)
    {
        cgroup_update_frozen_ptr = (cgroup_update_frozen_t)
          kallsyms_lookup_name("cgroup_update_frozen");
    }

    if (cgroup_update_frozen_ptr == NULL)
    {
        c_printk_error("couldn't find cgroup_update_frozen\n");
        return;
    }

    cgroup_update_frozen_ptr(cgrp);
}

void task_clear_jobctl_trapping(struct task_struct* task)
{
    typedef void (*task_clear_jobctl_trapping_t)(void*);
    static task_clear_jobctl_trapping_t task_clear_jobctl_trapping_ptr
      = NULL;

    if (task_clear_jobctl_trapping_ptr == NULL)
    {
        task_clear_jobctl_trapping_ptr = (task_clear_jobctl_trapping_t)
          kallsyms_lookup_name("task_clear_jobctl_trapping");
    }

    if (task_clear_jobctl_trapping_ptr == NULL)
    {
        c_printk_error("couldn't find "
                       "task_clear_jobctl_trapping\n");
        return;
    }

    task_clear_jobctl_trapping_ptr(task);
}

void task_clear_jobctl_pending(struct task_struct* task,
                               unsigned long mask)
{
    typedef void (*task_clear_jobctl_pending_t)(void*, unsigned long);
    static task_clear_jobctl_pending_t task_clear_jobctl_pending_ptr
      = NULL;

    if (task_clear_jobctl_pending_ptr == NULL)
    {
        task_clear_jobctl_pending_ptr = (task_clear_jobctl_pending_t)
          kallsyms_lookup_name("task_clear_jobctl_pending");
    }

    if (task_clear_jobctl_pending_ptr == NULL)
    {
        c_printk_error("couldn't find task_clear_jobctl_pending\n");
        return;
    }

    task_clear_jobctl_pending_ptr(task, mask);
}

int __group_send_sig_info(int i,
                          struct kernel_siginfo* sig,
                          struct task_struct* tsk)
{
    typedef int (*__group_send_sig_info_t)(int, void*, void*);
    static __group_send_sig_info_t __group_send_sig_info_ptr = NULL;

    if (__group_send_sig_info_ptr == NULL)
    {
        __group_send_sig_info_ptr = (__group_send_sig_info_t)
          kallsyms_lookup_name("__group_send_sig_info");
    }

    if (__group_send_sig_info_ptr == NULL)
    {
        c_printk_error("couldn't find __group_send_sig_info\n");
        return -1;
    }

    return __group_send_sig_info_ptr(i, sig, tsk);
}

void __wake_up_parent(struct task_struct* p, struct task_struct* parent)
{
    typedef void (*__wake_up_parent_t)(void*, void*);
    static __wake_up_parent_t __wake_up_parent_ptr = NULL;

    if (__wake_up_parent_ptr == NULL)
    {
        __wake_up_parent_ptr = (__wake_up_parent_t)kallsyms_lookup_name(
          "__wake_up_parent");
    }

    if (__wake_up_parent_ptr == NULL)
    {
        c_printk_error("couldn't find __wake_up_parent\n");
        return;
    }

    return __wake_up_parent_ptr(p, parent);
}

void task_work_run(void)
{
    typedef void (*task_work_run_t)(void);
    static task_work_run_t task_work_run_ptr = NULL;

    if (task_work_run_ptr == NULL)
    {
        task_work_run_ptr = (task_work_run_t)kallsyms_lookup_name(
          "task_work_run");
    }

    if (task_work_run_ptr == NULL)
    {
        c_printk_error("couldn't find task_work_run\n");
        return;
    }

    return task_work_run_ptr();
}

u64 nsec_to_clock_t(u64 x)
{
    typedef u64 (*nsec_to_clock_t_t)(u64);
    static nsec_to_clock_t_t nsec_to_clock_t_ptr = NULL;

    if (nsec_to_clock_t_ptr == NULL)
    {
        nsec_to_clock_t_ptr = (nsec_to_clock_t_t)kallsyms_lookup_name(
          "nsec_to_clock_t");
    }

    if (nsec_to_clock_t_ptr == NULL)
    {
        c_printk_error("couldn't find nsec_to_clock_t\n");
        return 0;
    }

    return nsec_to_clock_t_ptr(x);
}

int __vm_munmap(unsigned long start, size_t len, bool downgrade)
{
    int ret;
    struct mm_struct* mm = current->mm;
    LIST_HEAD(uf);

    if (down_write_killable(&mm->mmap_sem))
        return -EINTR;

    ret = __do_munmap(mm, start, len, &uf, downgrade);
    /*
     * Returning 1 indicates mmap_sem is downgraded.
     * But 1 is not legal return value of vm_munmap() and munmap(), reset
     * it to 0 before return.
     */
    if (ret == 1)
    {
        up_read(&mm->mmap_sem);
        ret = 0;
    }
    else
        up_write(&mm->mmap_sem);

    userfaultfd_unmap_complete(mm, &uf);
    return ret;
}
