#include "main.h"

void init_pattern_result(pattern_result_t* pattern_result, size_t count)
{
    if (count == 0)
    {
        pattern_result->addrs = NULL;
    }
    else
    {
        pattern_result->addrs = kmalloc(sizeof(uintptr_t) * count,
                                        GFP_KERNEL);
    }

    pattern_result->count = count;
}

bool realloc_pattern_result(pattern_result_t* pattern_result,
                            size_t add_count)
{
    pattern_result->count += add_count;

    if (pattern_result->addrs == NULL)
    {
        pattern_result->addrs = kmalloc(sizeof(uintptr_t)
                                          * pattern_result->count,
                                        GFP_KERNEL);
    }
    else
    {
        pattern_result->addrs = krealloc(pattern_result->addrs,
                                         sizeof(uintptr_t)
                                           * pattern_result->count,
                                         GFP_KERNEL);
    }

    if (pattern_result->addrs == NULL)
    {
        pattern_result->count -= add_count;
        return false;
    }

    return true;
}

void free_pattern_result(pattern_result_t* pattern_result)
{
    if (pattern_result)
    {
        kfree(pattern_result->addrs);
        pattern_result->addrs = NULL;
    }
}

void vm_flags_to_string(vm_area_t* vma, char* output, int size)
{
    if (size < 7)
    {
        c_printk("string buffer doesn't have enough memory"
                 " to write memory protection "
                 "flags\n");
        return;
    }

    strcpy(output, "------");

    if (vma->vm_flags & VM_READ)
    {
        output[0] = 'r';
    }
    if (vma->vm_flags & VM_WRITE)
    {
        output[1] = 'w';
    }
    if (vma->vm_flags & VM_EXEC)
    {
        output[2] = 'x';
    }
    if (vma->vm_flags & VM_SHARED)
    {
        output[3] = 's';
    }
    if (vma->vm_flags & VM_GROWSDOWN)
    {
        output[4] = 'd';
    }
    if (vma->vm_flags & VM_GROWSUP)
    {
        output[5] = 'g';
    }

    output[6] = '\0';
}

int vm_flags_to_prot(vm_area_t* vma)
{
    int prot;

    prot = PROT_NONE;

    if (vma->vm_flags & VM_READ)
    {
        prot |= PROT_READ;
    }
    if (vma->vm_flags & VM_WRITE)
    {
        prot |= PROT_WRITE;
    }
    if (vma->vm_flags & VM_EXEC)
    {
        prot |= PROT_EXEC;
    }
    if (vma->vm_flags & VM_GROWSDOWN)
    {
        prot |= PROT_GROWSDOWN;
    }
    if (vma->vm_flags & VM_GROWSUP)
    {
        prot |= PROT_GROWSUP;
    }

    return prot;
}

int prot_to_vm_flags(int prot)
{
    int flags;

    flags = VM_NONE;

    if (prot & PROT_READ)
    {
        flags |= VM_READ;
    }
    if (prot & PROT_WRITE)
    {
        flags |= VM_WRITE;
    }
    if (prot & PROT_EXEC)
    {
        flags |= VM_EXEC;
    }
    if (prot & PROT_GROWSDOWN)
    {
        flags |= VM_GROWSDOWN;
    }
    if (prot & PROT_GROWSUP)
    {
        flags |= VM_GROWSUP;
    }

    return flags;
}

// Find the part of virtual memory of the virtual address space range
// of the process that contains our virtual address.
int c_find_vma_from_task(task_t* task,
                         vm_area_t** vma_start,
                         uintptr_t wanted_addr)
{
    mm_t* mm;

    mm = get_task_mm_kthread(task);

    if (mm == NULL)
        goto out;

    *vma_start = mm->mmap;

    // Sometimes it can be null pointer as for systemdd process.
    if (*vma_start == NULL)
        goto out;

    // Loop through all the linkedlist of mapped memory areas until we
    // find our address.
    while (true)
    {
        // Waw we found it
        if (wanted_addr >= (*vma_start)->vm_start
            && wanted_addr < (*vma_start)->vm_end)
            break;

        // Go on the next mapped memory area
        *vma_start = (*vma_start)->vm_next;

        // Ouch we reached the end of the linked list, we didn't find
        // anything.
        if (*vma_start == NULL)
            break;
    }

    c_mmput(task, mm);

    return *vma_start == NULL ? 0 : 1;

out:
    c_mmput(task, mm);

    return 0;
}

int c_find_vma_from_task_str(task_t* task,
                             vm_area_t** vma_start,
                             const char* name)
{
    mm_t* mm;

    mm = get_task_mm_kthread(task);

    if (mm == NULL)
        goto out;

    *vma_start = mm->mmap;

    // Sometimes it can be null pointer as for systemdd process.
    if (*vma_start == NULL)
        goto out;

    // Loop through all the linkedlist of mapped memory areas until we
    // find our address.
    while (true)
    {
        if ((*vma_start)->vm_file)
        {
            if (!strcmp((const char*)(*vma_start)
                          ->vm_file->f_path.dentry->d_iname,
                        name))
            {
                break;
            }
        }

        // Go on the next mapped memory area
        *vma_start = (*vma_start)->vm_next;

        // Ouch we reached the end of the linked list, we didn't find
        // anything.
        if (*vma_start == NULL)
            break;
    }

    c_mmput(task, mm);

    return *vma_start == NULL ? 0 : 1;

out:
    c_mmput(task, mm);

    return 0;
}

void c_print_vmas(task_t* task)
{
    mm_t* mm;
    vm_area_t* vma_start;
    // Protect flags translated...
    char strflags[7];

    mm = get_task_mm_kthread(task);

    if (mm == NULL)
        return;

    vma_start = mm->mmap;

    // Sometimes it can be null pointer as for systemdd process.
    if (vma_start != NULL)
    {
        // Loop through all the linkedlist of mapped memory areas until we
        // find our address.
        while (true)
        {
            vm_flags_to_string(vma_start, strflags, sizeof(strflags));
            c_printk("%s(%i); start: 0x%lX; end: 0x%lX; current flags: "
                     "%s\n",
                     task->comm,
                     task->pid,
                     vma_start->vm_start,
                     vma_start->vm_end,
                     strflags);

            // Go on the next mapped memory area
            vma_start = vma_start->vm_next;

            // Ouch we reached the end of the linked list, we didn't find
            // anything.
            if (vma_start == NULL)
                break;
        }
    }

    c_mmput(task, mm);
}

task_t* find_task_from_addr(uintptr_t address)
{
    task_t* task;
    vm_area_t* vma;

    task = NULL;
    vma  = NULL;

    for_each_process(task)
    {
        if (c_find_vma_from_task(task, &vma, address))
        {
            break;
        }
    }

    return task;
}

int scan_pattern(uintptr_t start,
                 uintptr_t end,
                 char* pattern,
                 int len,
                 pattern_result_t* pattern_result)
{
    uintptr_t iter;
    char* pattern_c;
    int pattern_byte;
    int val_iter;

    val_iter = 0;

    if (pattern_result == NULL)
    {
        return 0;
    }

    while (start < end)
    {
        iter = start;

        for (pattern_c = pattern; pattern_c < pattern + len;)
        {
            // If it's an unknown byte, we skip the byte.
            if (*pattern_c == '?'
                && *(char*)((uintptr_t)pattern_c + 1) == '?')
            {
                iter++;
                // ??_
                pattern_c += 3;
                continue;
            }

            if (*pattern_c == '?'
                && *(char*)((uintptr_t)pattern_c + 1) == ' ')
            {
                iter++;
                // ?_
                pattern_c += 2;
                continue;
            }

            pattern_byte = hex_char_to_byte(*pattern_c,
                                            *(char*)((uintptr_t)pattern_c
                                                     + 1));

            // XX_
            pattern_c += 3;

            if (pattern_byte == 0x100)
            {
                c_printk("wrong pattern (%li): %s\n",
                         (uintptr_t)pattern_c - (uintptr_t)pattern,
                         pattern);
                return 0;
            }

            if (pattern_byte != (int)(*(unsigned char*)iter))
                goto dontmatch;

            iter++;
        }

        c_printk("found: %lX with pattern\n%s\n", iter, pattern);

        if (realloc_pattern_result(pattern_result, 1))
        {
            pattern_result->addrs[pattern_result->count - 1] = start;
        }
        else
        {
            return 0;
        }

    dontmatch:
        start++;
    }

    if (pattern_result->count == 0)
    {
        // c_printk("didn't find pattern\n%s\n", pattern);
        return 0;
    }
    else
    {
        return 1;
    }
}

int scan_task(task_t* task,
              char* pattern,
              int len,
              pattern_result_t* pattern_result)
{
    vm_area_t* vma;
    mm_t* mm;
    ptr_t copied_user_memory;
    mm_segment_t old_fs;

    copied_user_memory = NULL;

    old_fs = get_fs();
    set_fs(KERNEL_DS);

    mm = get_task_mm_kthread(task);

    if (mm == NULL)
    {
        c_printk("task %s(%i) has no mm struct!\n", task->comm, task->pid);
        return 0;
    }

    vma = mm->mmap;

    if (vma == NULL)
    {
        c_printk("task %s(%i) has no mmap struct!\n",
                 task->comm,
                 task->pid);
        c_mmput(task, mm);
        return 0;
    }

    while (true)
    {
        c_printk("vma scanning... %lX\n", vma->vm_start);

        copied_user_memory = kmalloc(vma->vm_end - vma->vm_start,
                                     GFP_KERNEL);

        if (c_copy_from_user(task,
                             copied_user_memory,
                             (ptr_t)vma->vm_start,
                             vma->vm_end - vma->vm_start))
        {
            kfree(copied_user_memory);
            c_printk("couldn't copy memory from task %s(%i) at %lX!\n",
                     task->comm,
                     task->pid,
                     vma->vm_start);
            break;
        }

        scan_pattern((uintptr_t)copied_user_memory,
                     (uintptr_t)copied_user_memory
                       + (vma->vm_end - vma->vm_start),
                     pattern,
                     len,
                     pattern_result);

        vma = vma->vm_next;
        kfree(copied_user_memory);

        if (vma == NULL)
            break;
    }

    c_mmput(task, mm);

    set_fs(old_fs);

    return pattern_result->count != 0;
}

int scan_kernel(char* start,
                char* end,
                char* pattern,
                int len,
                pattern_result_t* pattern_result)
{
    uintptr_t addr_start, addr_end;
    uintptr_t swap;
    int ret;
    size_t size_to_scan;
    mm_segment_t old_fs;

#ifndef __arch_um__
    int max_page_count;
    int page_count;
    uintptr_t scan_addr, scan_end_addr;
    pte_t* pte;
#endif

    // If pattern is too large
    if (len > PAGE_SIZE * 2)
    {
        c_printk("pattern too large\n");
        return 0;
    }

    addr_start = kallsyms_lookup_name(start);
    addr_end   = kallsyms_lookup_name(end);

    if (addr_start == 0 || addr_end == 0)
    {
        c_printk("wrong start - end addr: %lX - %lX\n",
                 addr_start,
                 addr_end);
        return 0;
    }

    if (addr_start > addr_end)
    {
        swap       = addr_start;
        addr_start = addr_end;
        addr_end   = swap;
    }

    size_to_scan = addr_end - addr_start;

#ifndef __arch_um__
    scan_addr      = align_address(addr_start, PAGE_SIZE);
    max_page_count = ((size_to_scan - 1) / PAGE_SIZE) + 1;
    page_count     = 0;
#endif

    c_printk("scanning with start - end addr: %lX - %lX (%li)\n",
             addr_start,
             addr_end,
             size_to_scan);

    old_fs = get_fs();
    set_fs(KERNEL_DS);

#ifndef __arch_um__
    while (page_count < max_page_count)
    {
        // Find first present page.
        while (1)
        {
            pte = get_pte(scan_addr);

            if (pte && (pte->pte & _PAGE_PRESENT))
            {
                break;
            }

            scan_addr += PAGE_SIZE;
            page_count++;

            // Ouch we reached our page count.
            if (page_count >= max_page_count)
                goto out;
        }

        scan_end_addr = scan_addr + PAGE_SIZE;

        // Stop if we reached our page count.
        while (page_count < max_page_count)
        {
            pte = get_pte(scan_end_addr);

            // If the page doesn't exist, let's scan with the lastest
            // scan_end_addr.
            if (pte == NULL || !(pte && (pte->pte & _PAGE_PRESENT)))
            {
                break;
            }

            scan_end_addr += PAGE_SIZE;
            page_count++;
        }

        // Let's scan now.
        ret = scan_pattern(scan_addr,
                           scan_end_addr,
                           pattern,
                           len,
                           pattern_result);
        // Next pointer.
        scan_addr = scan_end_addr;
    }

out:
    if (page_count != max_page_count)
    {
        c_printk_error("couldn't reach max page count\n");
        return 0;
    }
#else
    ret = scan_pattern(addr_start, addr_end, pattern, len, pattern_result);
#endif

    set_fs(old_fs);

    return pattern_result->count != 0;
}

uintptr_t map_base_task(task_t* task)
{
    mm_t* mm;
    uintptr_t result;

    if (task == NULL)
    {
        c_printk("wrong task struct\n");
        return 0;
    }

    mm = get_task_mm_kthread(task);

    if (mm == NULL)
    {
        c_printk("couldn't find base address of %s(%i)\n",
                 task->comm,
                 task->pid);
        return 0;
    }

    c_printk("found base address of %s(%i) (%lX)\n",
             task->comm,
             task->pid,
             mm->mmap_base);

    result = mm->mmap_base;

    c_mmput(task, mm);

    return result;
}

uintptr_t kernel_offset(void)
{
    // 0xffffffff81000000 is the usual base address of text section.
    return kallsyms_lookup_name("_text") - 0xffffffff81000000;
}

void change_vm_flags(vm_area_t* vma, int new_flags, int* old_flags)
{
    int all_prot_to_flags;

    all_prot_to_flags = VM_READ | VM_WRITE | VM_EXEC | VM_GROWSDOWN
                        | VM_GROWSUP;

    if (old_flags != NULL)
    {
        *old_flags = vm_flags_to_prot(vma);
    }

    // Do not delete special vm flags.
    vma->vm_flags &= ~all_prot_to_flags;
    vma->vm_flags |= prot_to_vm_flags(new_flags);
    vma->vm_page_prot = vm_get_page_prot(vma->vm_flags);
}

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
        c_printk("couldn't find vma_rb_erase\n");
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
        c_printk("couldn't find vm_area_alloc\n");
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
        c_printk("couldn't find vm_area_free\n");
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
        c_printk("couldn't find insert_vm_struct\n");
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
        c_printk("couldn't find ksys_mmap_pgoff\n");
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
        c_printk("couldn't find _do_fork\n");
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
        c_printk("couldn't find __switch_to\n");
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
        c_printk("couldn't find __switch_to_asm\n");
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
        c_printk("couldn't find __do_munmap\n");
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
        c_printk("couldn't find set_task_cpu\n");
        return;
    }

    set_task_cpu(p, cpu);
}

int c_munmap(task_t* task, uintptr_t start)
{
    mm_segment_t old_fs;
    struct mm_struct* mm;
    struct vm_area_struct *vma, *prev_vma, *next_vma;
    int ret;

    vma = NULL;
    ret = -1;

    old_fs = get_fs();
    set_fs(KERNEL_DS);

    mm = get_task_mm_kthread(task);

    if (mm == NULL)
    {
        c_printk_error("couldn't find mm from task %i\n", task->pid);
        goto out;
    }

    down_write(&mm->mmap_sem);

    if (c_find_vma_from_task(task, &vma, start))
    {
        prev_vma = vma->vm_prev;
        next_vma = vma->vm_next;

        vma_rb_erase(vma, &mm->mm_rb);

        if (prev_vma != NULL)
        {
            prev_vma->vm_next = next_vma;
        }
        else
        {
            mm->mmap = next_vma;
        }

        if (next_vma != NULL)
        {
            next_vma->vm_prev = prev_vma;
        }

        vm_area_free(vma);
        ret = 0;
    }

    up_write(&mm->mmap_sem);

    c_mmput(task, mm);

out:

    set_fs(old_fs);

    return ret;
}

void c_mmput(task_t* task, mm_t* mm)
{
    if (task->active_mm == NULL && mm)
        mmput(mm);
}

uintptr_t align_address(uintptr_t address, size_t size)
{
    uintptr_t aligned_address;

    aligned_address = address - (address % size);

    return aligned_address;
}

void alloc_buffer(size_t size, buffer_t* buffer)
{
    buffer->addr = kmalloc(size, GFP_KERNEL);
    buffer->size = size;
}

void free_buffer(buffer_t* buffer)
{
    if (buffer->addr != NULL)
    {
        kfree(buffer->addr);
        buffer->addr = NULL;
        buffer->size = 0;
    }
}

int realloc_buffer(size_t size, buffer_t* buffer)
{
    buffer->size += size;
    buffer->addr = krealloc(buffer->addr, buffer->size, GFP_KERNEL);

    if (buffer->addr != NULL)
    {
        return 1;
    }

    buffer->size -= size;
    return 0;
}

void init_buffer(buffer_t* buffer)
{
    buffer->size = 0;
    buffer->addr = NULL;
}

mm_t* get_task_mm_kthread(task_t* task)
{
    mm_t* mm;

    mm = get_task_mm(task);

    // Kernel thread?
    if (mm == NULL)
        mm = task->active_mm;

    return mm;
}

unsigned long
c_copy_to_user(task_t* task, ptr_t to, ptr_t from, size_t size)
{
    mm_t* mm;
    page_t* page;
    unsigned long result;
    bool should_up_write;
    mm_segment_t old_fs;
    size_t size_to_copy;
    uintptr_t real_addr, user_align_addr;
    uintptr_t shifted;

    // So we can access anywhere we can in user space.
    old_fs = get_fs();
    set_fs(KERNEL_DS);

    should_up_write = false;
    result          = size;

    mm = get_task_mm_kthread(task);

    if (mm == NULL)
    {
        goto out;
    }

    down_write(&mm->mmap_sem);
    should_up_write = true;

    user_align_addr = (uintptr_t)align_address((uintptr_t)to, PAGE_SIZE);
    shifted         = (uintptr_t)to % PAGE_SIZE;

    result = 0;

    while (size > 0)
    {
        if (get_user_pages_remote(task,
                                  mm,
                                  user_align_addr,
                                  1,
                                  FOLL_FORCE | FOLL_REMOTE | FOLL_SPLIT
                                    | FOLL_WRITE,
                                  &page,
                                  NULL,
                                  NULL)
            <= 0)
        {
            result = size;
            goto out;
        }

        real_addr = (uintptr_t)kmap(page) + shifted;

        size_to_copy = PAGE_SIZE - shifted;

        if (size_to_copy > size)
        {
            size_to_copy = size;
        }

        result += copy_to_user((ptr_t)real_addr, from, size_to_copy);

        kunmap(page);

        if (result != 0)
        {
            result = size;
            put_page(page);
            goto out;
        }

        put_page(page);

        size -= size_to_copy;

        // We done the first page, we can go by copying now.
        shifted = 0;
        user_align_addr += PAGE_SIZE;
    }

    if (size != 0)
    {
        c_printk_error("error on algorithm, contact a dev\n");
        result = size;
    }

out:
    if (should_up_write)
    {
        up_write(&mm->mmap_sem);
        c_mmput(task, mm);
    }

    set_fs(old_fs);

    return result;
}

unsigned long
c_copy_from_user(task_t* task, ptr_t to, ptr_t from, size_t size)
{
    mm_t* mm;
    page_t* page;
    unsigned long result;
    bool should_up_read;
    mm_segment_t old_fs;
    size_t size_to_copy;
    uintptr_t real_addr, user_align_addr;
    uintptr_t shifted;

    // So we can access anywhere we can in user space.
    old_fs = get_fs();
    set_fs(KERNEL_DS);

    should_up_read = false;
    result         = size;

    mm = get_task_mm_kthread(task);

    if (mm == NULL)
    {
        goto out;
    }

    down_read(&mm->mmap_sem);
    should_up_read = true;

    user_align_addr = (uintptr_t)align_address((uintptr_t)from,
                                               PAGE_SIZE);
    shifted         = (uintptr_t)from % PAGE_SIZE;

    result = 0;

    while (size > 0)
    {
        if (get_user_pages_remote(task,
                                  mm,
                                  user_align_addr,
                                  1,
                                  FOLL_FORCE | FOLL_REMOTE | FOLL_SPLIT
                                    | FOLL_WRITE,
                                  &page,
                                  NULL,
                                  NULL)
            <= 0)
        {
            result = size;
            goto out;
        }

        real_addr = (uintptr_t)page_address(page) + shifted;

        size_to_copy = PAGE_SIZE - shifted;

        if (size_to_copy > size)
        {
            size_to_copy = size;
        }

        result += copy_from_user(to, (ptr_t)real_addr, size_to_copy);

        kunmap(page);

        if (result != 0)
        {
            result = size;
            put_page(page);
            goto out;
        }

        put_page(page);

        size -= size_to_copy;

        // We done the first page, we can go by copying now.
        shifted = 0;
        user_align_addr += PAGE_SIZE;
    }

    if (size != 0)
    {
        c_printk_error("error on algorithm, contact a dev\n");
        result = size;
    }

out:
    if (should_up_read)
    {
        up_read(&mm->mmap_sem);
        c_mmput(task, mm);
    }

    set_fs(old_fs);

    return result;
}

task_t* find_task_from_pid(pid_t pid)
{
    task_t* task;

    for_each_process(task)
    {
        if (task->pid == pid)
        {
            return task;
        }
    }

    return NULL;
}

#ifndef __arch_um__

pte_t* get_pte(uintptr_t address)
{
    unsigned int level;
    return lookup_address(address, &level);
}

pteval_t get_page_flags(uintptr_t addr)
{
    pte_t* pte;

    pte = get_pte(addr);

    if (pte)
    {
        return pte->pte;
    }
    else
    {
        return -1;
    }
}

pteval_t set_page_flags(uintptr_t addr, pteval_t val)
{
    pteval_t old_pte_val;
    pte_t* pte;

    pte = get_pte(addr);

    if (pte)
    {
        old_pte_val = pte->pte;
        pte->pte    = val;
        return old_pte_val;
    }
    else
    {
        return -1;
    }
}

#else

void set_current(struct task_struct* task)
{
    static struct cpu_task* cpu_tasks = NULL;
    static int* __userspace_pid = NULL;

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

/* Used for debugging it gets optimized with gcc flags anyway */
task_t* debug_get_current(void)
{
    return get_current();
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
        c_printk("couldn't find vmalloc_exec\n");
        return NULL;
    }

    return p_vmalloc_exec(size);
}
