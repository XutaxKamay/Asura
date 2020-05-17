#include "main.h"

#define GUP_FLAGS FOLL_FORCE | FOLL_WRITE

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

task_t* find_task_from_addr(uintptr_t special_address)
{
    task_t* task;
    vm_area_t* vma;

    task = NULL;
    vma  = NULL;

    for_each_process(task)
    {
        if (c_find_vma_from_task(task, &vma, special_address))
        {
            break;
        }
    }

    return task;
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

mm_t* get_task_mm_kthread(task_t* task)
{
    mm_t* mm;

    mm = get_task_mm(task);

    // Kernel thread?
    if (mm == NULL)
    {
        mm = task->active_mm;
    }

    return mm;
}

task_t* find_task_from_pid(pid_t pid)
{
    task_t* task;

    for_each_process(task)
    {
        if (task->pid == pid)
        {
            // Add refcount
            task = get_task_struct(task);
            return task;
        }
    }

    return task;
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

/**
 * task_struct and mm_struct utils
 */

/**
 * Finds the part of virtual memory of the virtual address space range
 * of the process that contains our virtual address.
 */
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
    return kallsyms_lookup_name("_text") - KERNEL_USUAL_TEXT_SECTION;
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

vm_area_t* c_mmap(task_t* task, uintptr_t address, int prot)
{
    mm_t* mm;
    vm_area_t* vma;
    bool should_up_write;

    should_up_write = false;
    vma             = NULL;

    mm = get_task_mm_kthread(task);

    if (mm == NULL)
    {
        c_printk("couldn't find mm from task %i\n", task->pid);
        goto out;
    }

    down_write(&mm->mmap_sem);
    should_up_write = true;

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
    if (should_up_write)
    {
        up_write(&mm->mmap_sem);
        c_mmput(task, mm);
    }

    return vma;
}

void c_mmput(task_t* task, mm_t* mm)
{
    // Don't do that on kernel threads
    if (mm && (task->mm == task->active_mm))
        mmput(mm);
}

/**
 * Taken from linux kernel
 */
int c___vm_munmap(task_t* task,
                  unsigned long start,
                  size_t len,
                  bool downgrade)
{
    int ret;
    struct mm_struct* mm = task->mm;
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

/**
 * End of memory mapping & task utils
 */

uintptr_t align_address(uintptr_t address, size_t size)
{
    uintptr_t aligned_address;

    aligned_address = address - (address % size);

    return aligned_address;
}

unsigned long
c_copy_to_user(task_t* task, ptr_t to, ptr_t from, size_t size)
{
    mm_t* mm;
    page_t** pages;
    unsigned long result;
    mm_segment_t old_fs;
    size_t size_to_copy;
    uintptr_t real_addr, user_align_addr;
    uintptr_t shifted;
    int nr_pages, nr_page;
    uintptr_t copied_bytes;

    // So we can access anywhere we can in user space.
    old_fs = get_fs();
    set_fs(KERNEL_DS);

    result = size;

    mm = get_task_mm_kthread(task);

    if (mm == NULL)
    {
        goto out;
    }

    down_write(&mm->mmap_sem);

    user_align_addr = (uintptr_t)align_address((uintptr_t)to, PAGE_SIZE);
    shifted         = (uintptr_t)to % PAGE_SIZE;

    /**
     * We might add a page if there is too much shifting
     * For example :
     *
     *  PAGE_SIZE = 0x1000
     *  user_align_addr = 0x0
     *  size = 0x1002
     *  shifted = 0xFFF
     *
     *  Theorically, it should be two pages,
     *  but there is three pages.
     *
     *  What happens?
     *
     *  -> = becomes
     *
     *  1st page:
     *  user_align_addr = 0xFFF -> 0x1000
     *  size = 0x1002 -> 0x1001
     *  shifted = 0xFFF -> 0x0
     *
     *  2nd page:
     *  user_align_addr = 0x1000 -> 0x2000
     *  size = 0x1001 -> 0x1
     *  shifted = 0x0
     *
     *  size == 1
     *  So then...We need a third page.
     *
     *  3rd page:
     *  user_align_addr = 0x2000
     *  size = 0x1 -> 0x0
     *  shifted = 0x0
     *
     *  Done.
     */

    nr_pages        = (((size + shifted) - 1) / PAGE_SIZE) + 1;

    pages = kmalloc(nr_pages * sizeof(ptr_t), GFP_KERNEL);

    if (current != task)
    {
        if (get_user_pages_remote(task,
                                  mm,
                                  user_align_addr,
                                  nr_pages,
                                  GUP_FLAGS,
                                  pages,
                                  NULL,
                                  NULL)
            <= 0)
        {
            goto out_sem;
        }
    }
    else
    {
        if (get_user_pages_fast(user_align_addr,
                                nr_pages,
                                GUP_FLAGS,
                                pages)
            <= 0)
        {
            goto out_sem;
        }
    }

    for (nr_page = 0; nr_page < nr_pages; nr_page++)
    {
        real_addr = (uintptr_t)kmap(pages[nr_page]) + shifted;

        size_to_copy = PAGE_SIZE - shifted;

        if (size_to_copy > result)
        {
            size_to_copy = result;
        }

        copied_bytes = copy_to_user((ptr_t)real_addr, from, size_to_copy);

        result -= (size_to_copy - copied_bytes);

        if (copied_bytes != 0)
        {
            goto out_sem;
        }

        if (result <= 0)
        {
            break;
        }

        // We done the first page, we can go by copying now.
        if (shifted != 0)
            shifted = 0;

        user_align_addr += PAGE_SIZE;
    }

    if (result != 0)
    {
        c_printk_error("error on algorithm, contact a dev (%li %li "
                       "%i %p)\n",
                       result,
                       size,
                       nr_pages,
                       to);
    }

out_sem:
    up_write(&mm->mmap_sem);
    c_mmput(task, mm);
    release_pages(pages, nr_pages);
    kfree(pages);

out:
    set_fs(old_fs);

    return result;
}

unsigned long
c_copy_from_user(task_t* task, ptr_t to, ptr_t from, size_t size)
{
    mm_t* mm;
    page_t** pages;
    unsigned long result;
    mm_segment_t old_fs;
    size_t size_to_copy;
    uintptr_t real_addr, user_align_addr;
    uintptr_t shifted;
    int nr_pages, nr_page;
    uintptr_t copied_bytes;

    // So we can access anywhere we can in user space.
    old_fs = get_fs();
    set_fs(KERNEL_DS);

    result = size;

    mm = get_task_mm_kthread(task);

    if (mm == NULL)
    {
        goto out;
    }

    down_read(&mm->mmap_sem);

    user_align_addr = (uintptr_t)align_address((uintptr_t)from,
                                               PAGE_SIZE);
    shifted         = (uintptr_t)from % PAGE_SIZE;

    /**
     * We might add a page if there is too much shifting
     * For example :
     *
     *  PAGE_SIZE = 0x1000
     *  user_align_addr = 0x0
     *  size = 0x1002
     *  shifted = 0xFFF
     *
     *  Theorically, it should be two pages,
     *  but there is three pages.
     *
     *  What happens?
     *
     *  -> = becomes
     *
     *  1st page:
     *  user_align_addr = 0xFFF -> 0x1000
     *  size = 0x1002 -> 0x1001
     *  shifted = 0xFFF -> 0x0
     *
     *  2nd page:
     *  user_align_addr = 0x1000 -> 0x2000
     *  size = 0x1001 -> 0x1
     *  shifted = 0x0
     *
     *  size == 1
     *  So then...We need a third page.
     *
     *  3rd page:
     *  user_align_addr = 0x2000
     *  size = 0x1 -> 0x0
     *  shifted = 0x0
     *
     *  Done.
     */

    nr_pages        = (((size + shifted) - 1) / PAGE_SIZE) + 1;

    pages = kmalloc(nr_pages * sizeof(ptr_t), GFP_KERNEL);

    if (current != task)
    {
        if (get_user_pages_remote(task,
                                  mm,
                                  user_align_addr,
                                  nr_pages,
                                  GUP_FLAGS,
                                  pages,
                                  NULL,
                                  NULL)
            <= 0)
        {
            goto out_sem;
        }
    }
    else
    {
        if (get_user_pages_fast(user_align_addr,
                                nr_pages,
                                GUP_FLAGS,
                                pages)
            <= 0)
        {
            goto out_sem;
        }
    }

    for (nr_page = 0; nr_page < nr_pages; nr_page++)
    {
        real_addr = (uintptr_t)kmap(pages[nr_page]) + shifted;

        size_to_copy = PAGE_SIZE - shifted;

        if (size_to_copy > result)
        {
            size_to_copy = result;
        }

        copied_bytes = copy_from_user(to, (ptr_t)real_addr, size_to_copy);

        result -= (size_to_copy - copied_bytes);

        if (copied_bytes != 0)
        {
            goto out_sem;
        }

        if (result <= 0)
        {
            break;
        }

        // We done the first page, we can go by copying now.
        if (shifted != 0)
            shifted = 0;

        user_align_addr += PAGE_SIZE;
    }

    if (result != 0)
    {
        c_printk_error("error on algorithm, contact a dev (%li %li "
                       "%i %p)\n",
                       result,
                       size,
                       nr_pages,
                       from);
    }

out_sem:
    up_read(&mm->mmap_sem);
    c_mmput(task, mm);
    release_pages(pages, nr_pages);
    kfree(pages);

out:
    set_fs(old_fs);

    return result;
}

/**
 * Paging stuff
 */

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

task_t** get_current_task_ptr(void)
{
#ifndef __arch_um__
    return (task_t**)(my_cpu_offset + (uintptr_t)&current_task);
#else
    return &current;
#endif
}

void switch_to_task(task_t* task)
{
    __switch_to(current, task);
}

static int find_sym_callback(temp_symbol_t* sym,
                             const char* name,
                             struct module* mod,
                             unsigned long addr)
{
    if (sym->addr)
    {
        return 1;
    }

    if (!strcmp(sym->name, name))
    {
        sym->addr = addr;
    }

    return 0;
}

uintptr_t c_find_sym_addr(const char* name)
{
    temp_symbol_t temp_symbol;

    temp_symbol.name = name;
    temp_symbol.addr = 0;

    kallsyms_on_each_symbol((void*)find_sym_callback, &temp_symbol);

    return temp_symbol.addr;
}
