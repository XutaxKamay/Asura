#include "memutils.h"

void vm_flags_to_string(struct vm_area_struct* vma, char* output, int size)
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

int vm_flags_to_prot(struct vm_area_struct* vma)
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
int c_find_vma_from_task(struct task_struct* task,
                         struct vm_area_struct** vma_start,
                         uintptr_t wanted_addr)
{
    struct mm_struct* mm;

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

    return *vma_start == NULL ? 0 : 1;

out:
    return 0;
}

int c_find_vma_from_task_str(struct task_struct* task,
                             struct vm_area_struct** vma_start,
                             const char* name)
{
    struct mm_struct* mm;

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

    return *vma_start == NULL ? 0 : 1;

out:
    return 0;
}

void c_print_vmas(struct task_struct* task)
{
    struct mm_struct* mm;
    struct vm_area_struct* vma_start;
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
}

struct task_struct* find_task_from_addr(uintptr_t address)
{
    struct task_struct* task;
    struct vm_area_struct* vma;

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
                 struct buffer_struct* buf)
{
    uintptr_t iter;
    char* pattern_c;
    int pattern_byte;
    int val_iter;
    mm_segment_t old_fs;

    val_iter = 0;
    old_fs   = get_fs();

    if (buf == NULL)
    {
        return 0;
    }

    set_fs(KERNEL_DS);

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
                set_fs(old_fs);

                c_printk("wrong pattern (%li): %s\n",
                         (uintptr_t)pattern_c - (uintptr_t)pattern,
                         pattern);
                return 0;
            }

            if (copy_from_user(&val_iter, (ptr_t)iter, sizeof(char)))
                goto dontmatch;

            if (pattern_byte != val_iter)
                goto dontmatch;

            iter++;
        }

        c_printk("found: %lX with pattern\n%s\n", iter, pattern);

        if (buf->addr == NULL)
        {
            alloc_buffer(sizeof(ptr_t), buf);

            if (buf->addr == NULL)
            {
                set_fs(old_fs);

                c_printk("kmalloc failed: with pattern\n%s\n", pattern);
                return 0;
            }

            *(uintptr_t*)(buf->addr) = start;
        }
        else
        {
            realloc_buffer(sizeof(ptr_t), buf);

            if (buf->addr == NULL)
            {
                set_fs(old_fs);

                c_printk("krealloc failed: with pattern\n%s\n", pattern);

                return 0;
            }

            *(uintptr_t*)(buf->addr + buf->size - sizeof(ptr_t)) = start;
        }

    dontmatch:
        start++;
    }

    set_fs(old_fs);

    if (buf->addr == NULL)
    {
        c_printk("didn't find pattern\n%s\n", pattern);
        return 0;
    }
    else
    {
        return 1;
    }
}

int scan_task(struct task_struct* task,
              char* pattern,
              int len,
              struct buffer_struct* buf)
{
    struct vm_area_struct* vma;
    struct mm_struct* mm;
    ptr_t copied_user_memory;

    copied_user_memory = NULL;

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
                     buf);

        vma = vma->vm_next;
        kfree(copied_user_memory);

        if (vma == NULL)
            break;
    }

    return 0;
}

int scan_kernel(char* start,
                char* end,
                char* pattern,
                int len,
                struct buffer_struct* buf)
{
    uintptr_t addr_start, addr_end;

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
        uintptr_t swap = addr_start;
        addr_start     = addr_end;
        addr_end       = swap;
    }

    c_printk("scanning with start - end addr: %lX - %lX\n",
             addr_start,
             addr_end);

    return scan_pattern(addr_start, addr_end, pattern, len, buf);
}

uintptr_t map_base_task(struct task_struct* task)
{
    struct mm_struct* mm;

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

    return mm->mmap_base;
}

uintptr_t kernel_offset(void)
{
    // 0xffffffff81000000 is the usual base address of text section.
    return kallsyms_lookup_name("_text") - 0xffffffff81000000;
}

void change_vm_flags(struct vm_area_struct* vma,
                     int new_flags,
                     int* old_flags)
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

int remote_mprotect(struct task_struct* task,
                    uintptr_t address,
                    int new_flags,
                    int* old_flags)
{
    struct vm_area_struct* vma_start;
    int ret;

    vma_start = NULL;

    if (c_find_vma_from_task(task, &vma_start, address))
    {
        change_vm_flags(vma_start, new_flags, old_flags);
        ret = 1;
        goto out;
    }

    ret = 0;

out:
    return ret;
}

struct vm_area_struct* remote_mmap(struct task_struct* task,
                                   uintptr_t address,
                                   int prot)
{
    struct mm_struct* mm;
    struct vm_area_struct* vma;

    vma = NULL;

    mm = get_task_mm_kthread(task);

    if (mm == NULL)
    {
        c_printk("couldn't find mm from task %i\n", task->pid);
        goto out;
    }

    down_write(&mm->mmap_sem);

    vma = vm_area_alloc(mm);

    if (vma == NULL)
    {
        c_printk("couldn't allocate vma from task %i\n", task->pid);
        goto out;
    }

    vma_set_anonymous(vma);

    vma->vm_start     = address;
    vma->vm_end       = address + PAGE_SIZE;
    vma->vm_flags     = prot_to_vm_flags(prot) | mm->def_flags;
    vma->vm_page_prot = vm_get_page_prot(vma->vm_flags);

    INIT_LIST_HEAD(&vma->anon_vma_chain);

    if (insert_vm_struct(mm, vma) < 0)
    {
        vm_area_free(vma);
        c_printk("couldn't insert vma in mm struct from task %i\n",
                 task->pid);
        goto out;
    }

out:
    up_write(&mm->mmap_sem);
    return vma;
}

/* Redefine functions */

struct vm_area_struct* vm_area_alloc(struct mm_struct* mm)
{
    typedef struct vm_area_struct* (*vm_area_alloc_t)(struct mm_struct*);
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

void vm_area_free(struct vm_area_struct* vma)
{
    typedef void (*vm_area_free_t)(struct vm_area_struct*);
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

int insert_vm_struct(struct mm_struct* mm, struct vm_area_struct* vma)
{
    typedef int (*insert_vm_struct_t)(struct mm_struct*,
                                      struct vm_area_struct*);

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

uintptr_t align_address(uintptr_t address, size_t size)
{
    uintptr_t aligned_address;

    aligned_address = address - (address % size);

    return aligned_address;
}

void alloc_buffer(size_t size, struct buffer_struct* buffer)
{
    buffer->addr = kmalloc(size, GFP_KERNEL);
    buffer->size = size;
}

void free_buffer(struct buffer_struct* buffer)
{
    if (buffer->addr != NULL)
    {
        kfree(buffer->addr);
        buffer->addr = NULL;
        buffer->size = 0;
    }
}

int realloc_buffer(size_t size, struct buffer_struct* buffer)
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

void init_buffer(struct buffer_struct* buffer)
{
    buffer->size = 0;
    buffer->addr = NULL;
}

struct mm_struct* get_task_mm_kthread(struct task_struct* task)
{
    struct mm_struct* mm;

    mm = get_task_mm(task);

    // Kernel thread?
    if (mm == NULL)
        mm = task->active_mm;

    return mm;
}

unsigned long c_copy_to_user(struct task_struct* task,
                             ptr_t to,
                             ptr_t from,
                             size_t size)
{
    struct mm_struct* mm;
    struct page** page;

    unsigned long result;
    ptr_t realaddr;
    uintptr_t alignedaddress;
    uintptr_t shifted;
    int nb_pages;
    int nb_page;
    size_t size_to_copy;
    bool should_up_write;

    should_up_write = false;
    nb_pages        = ((size - 1) / PAGE_SIZE) + 1;
    result          = size;

    alignedaddress = align_address((uintptr_t)to, PAGE_SIZE);
    shifted        = (uintptr_t)to - alignedaddress;

    page = (struct page**)kmalloc(nb_pages * sizeof(struct page*),
                                  GFP_KERNEL);

    mm = get_task_mm_kthread(task);

    if (mm == NULL)
    {
        goto out;
    }

    down_write(&mm->mmap_sem);
    should_up_write = true;

    if (get_user_pages_remote(task,
                              mm,
                              alignedaddress,
                              nb_pages,
                              FOLL_FORCE,
                              page,
                              NULL,
                              NULL)
        <= 0)
    {
        goto out;
    }

    result = 0;

    for (nb_page = 0; nb_page < nb_pages; nb_page++)
    {
        if (nb_page == 0)
        {
            realaddr = (ptr_t)((uintptr_t)page_address(page[nb_page])
                               + shifted);

            if (size > PAGE_SIZE - shifted)
            {
                size_to_copy = PAGE_SIZE - shifted;
            }
            else
            {
                size_to_copy = size;
            }

            size -= size_to_copy;
        }
        else
        {
            realaddr = page_address(page[nb_page]);

            if (size > PAGE_SIZE)
            {
                size_to_copy = PAGE_SIZE;
            }
            else
            {
                size_to_copy = size;
            }

            size -= size_to_copy;
        }

        result += copy_to_user(realaddr, from, size_to_copy);
    }

    if (size != 0)
    {
        BUG();
    }

out:
    if (should_up_write)
        up_write(&mm->mmap_sem);
    kfree(page);

    return result;
}

unsigned long c_copy_from_user(struct task_struct* task,
                               ptr_t to,
                               ptr_t from,
                               size_t size)
{
    struct mm_struct* mm;
    struct page** page;

    unsigned long result;
    ptr_t realaddr;
    uintptr_t alignedaddress;
    uintptr_t shifted;
    int nb_pages;
    int nb_page;
    size_t size_to_copy;
    bool should_up_read;

    should_up_read = false;

    nb_pages = ((size - 1) / PAGE_SIZE) + 1;
    result   = size;

    alignedaddress = align_address((uintptr_t)from, PAGE_SIZE);
    shifted        = (uintptr_t)from - alignedaddress;

    page = (struct page**)kmalloc(nb_pages * sizeof(struct page*),
                                  GFP_KERNEL);

    mm = get_task_mm_kthread(task);

    if (mm == NULL)
    {
        goto out;
    }

    down_read(&mm->mmap_sem);
    should_up_read = true;

    if (get_user_pages_remote(task,
                              mm,
                              alignedaddress,
                              nb_pages,
                              FOLL_FORCE,
                              page,
                              NULL,
                              NULL)
        <= 0)
    {
        goto out;
    }

    result = 0;

    for (nb_page = 0; nb_page < nb_pages; nb_page++)
    {
        if (nb_page == 0)
        {
            realaddr = (ptr_t)((uintptr_t)page_address(page[nb_page])
                               + shifted);

            if (size > PAGE_SIZE - shifted)
            {
                size_to_copy = PAGE_SIZE - shifted;
            }
            else
            {
                size_to_copy = size;
            }

            size -= size_to_copy;
        }
        else
        {
            realaddr = page_address(page[nb_page]);

            if (size > PAGE_SIZE)
            {
                size_to_copy = PAGE_SIZE;
            }
            else
            {
                size_to_copy = size;
            }

            size -= size_to_copy;
        }

        result += copy_from_user(to, realaddr, size_to_copy);
    }

    if (size != 0)
    {
        BUG();
    }

out:
    if (should_up_read)
        up_read(&mm->mmap_sem);

    kfree(page);

    return result;
}

struct task_struct* find_task_from_pid(pid_t pid)
{
    struct task_struct* task;

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

#endif
