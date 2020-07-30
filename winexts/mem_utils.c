#include "main.h"

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
    if (buffer == NULL)
    {
        alloc_buffer(size, buffer);
        return 1;
    }
    else
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
}

void init_buffer(buffer_t* buffer)
{
    buffer->size = 0;
    buffer->addr = NULL;
}

/**
 * task_struct and mm_struct utils
 */

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

    /**
     * VM_MAY*
     */
    prot |= vma->vm_flags
            & (C_PROT_MAY_EXEC | C_PROT_MAY_READ | C_PROT_MAY_WRITE
               | C_PROT_MAY_SHARE);

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

    /**
     * VM_MAY*
     */
    flags |= prot
             & (C_PROT_MAY_EXEC | C_PROT_MAY_READ | C_PROT_MAY_WRITE
                | C_PROT_MAY_SHARE);

    return flags;
}

task_t* find_task_from_addr(uintptr_t special_address)
{
    task_t* task;
    vm_area_t* vma;

    vma = NULL;

    for_each_process(task)
    {
        if (c_find_vma_from_task(task, &vma, special_address))
        {
            //             if (task != current)
            //             {
            //                 get_task_struct(task);
            //             }

            return task;
        }
    }

    return NULL;
}

mm_t* get_task_mm_kthread(task_t* task)
{
    mm_t* mm;

    //     mm = get_task_mm(task);

    mm = task->mm;

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
            /**
             * Don't need to do that for current?
             */
            //             if (task != current)
            //             {
            //                 get_task_struct(task);
            //             }

            return task;
        }
    }

    return NULL;
}

void c_put_task_struct(task_t* task)
{
    /**
     * Don't need to do that for current?
     */
    //     if (task != current)
    //     {
    //         put_task_struct(task);
    //     }
}

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

    return *vma_start == NULL ? 0 : 1;

out:

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

    return *vma_start == NULL ? 0 : 1;

out:

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
    struct vm_area_struct* vma /*, *prev_vma, *next_vma*/;
    LIST_HEAD(uf);
    int ret;
    // long nrpages;

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
        /*prev_vma = vma->vm_prev;
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

        nrpages = vma_pages(vma);

        vm_stat_account(mm, vma->vm_flags, -nrpages);
        mm->map_count--;
        mpol_put(vma_policy(vma));
        // vm_unacct_memory(nrpages);

        vm_area_free(vma);*/

        ret = do_munmap(mm,
                        vma->vm_start,
                        vma->vm_end - vma->vm_start,
                        &uf);
    }

    /**
     * Debugging
     */
    // validate_mm(mm);

    up_write(&mm->mmap_sem);

out:

    set_fs(old_fs);

    return ret;
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

vm_area_t* c_mmap(task_t* task,
                  uintptr_t address,
                  uintptr_t size,
                  int prot)
{
    mm_t* mm;
    vm_area_t* vma;
    bool should_up_write;
    mm_segment_t old_fs;

    old_fs = get_fs();
    set_fs(KERNEL_DS);

    if ((address % PAGE_SIZE) != 0)
    {
        c_printk_error("unaligned address for c_mmap task %i\n",
                       task->pid);
        goto out;
    }

    if ((size % PAGE_SIZE) != 0)
    {
        c_printk_error("unaligned size for c_mmap task %i\n", task->pid);
        goto out;
    }

    should_up_write = false;
    vma             = NULL;

    if (c_find_vma_from_task(task, &vma, address))
    {
        goto out;
    }

    mm = get_task_mm_kthread(task);

    if (mm == NULL)
    {
        c_printk_error("couldn't find mm from task %i\n", task->pid);
        goto out;
    }

    down_write(&mm->mmap_sem);
    should_up_write = true;

    vma = vm_area_alloc(mm);

    if (vma == NULL)
    {
        c_printk_error("couldn't allocate vma from task %i\n", task->pid);
        goto out;
    }

    vma_set_anonymous(vma);

    vma->vm_start     = address;
    vma->vm_end       = address + size;
    vma->vm_flags     = prot_to_vm_flags(prot) | mm->def_flags;
    vma->vm_page_prot = vm_get_page_prot(vma->vm_flags);

    if (insert_vm_struct(mm, vma) < 0)
    {
        vm_area_free(vma);
        vma = NULL;
        c_printk_error("couldn't insert vma in mm struct from task %i\n",
                       task->pid);
        goto out;
    }

    vm_stat_account(mm, vma->vm_flags, (size / PAGE_SIZE) + 1);

out:
    /**
     * Debugging
     */
    // validate_mm(mm);

    if (should_up_write)
    {
        up_write(&mm->mmap_sem);
    }

    set_fs(old_fs);

    return vma;
}

void c_mmput(task_t* task, mm_t* mm)
{
    // Don't do that on kernel threads
    //     if (mm && (task->mm == task->active_mm))
    //         mmput(mm);
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
    struct mm_struct* mm = get_task_mm_kthread(task);
    LIST_HEAD(uf);

    if (mm == NULL)
    {
        return -EACCES;
    }

    if (down_write_killable(&mm->mmap_sem))
    {
        c_mmput(task, mm);
        return -EINTR;
    }

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
    c_mmput(task, mm);
    return ret;
}

int c_vma_count(mm_t* mm)
{
    vm_area_t* vma = mm->mmap;
    int count      = 0;

    while (vma != NULL)
    {
        count++;
        vma = vma->vm_next;
    }

    return count;
}

/**
 * Credits to linux kernel for mprotect
 */

static int prot_none_pte_entry(pte_t* pte,
                               unsigned long addr,
                               unsigned long next,
                               struct mm_walk* walk)
{
    return pfn_modify_allowed(pte_pfn(*pte),
                              *(pgprot_t*)(walk->private)) ?
             0 :
             -EACCES;
}

static int prot_none_hugetlb_entry(pte_t* pte,
                                   unsigned long hmask,
                                   unsigned long addr,
                                   unsigned long next,
                                   struct mm_walk* walk)
{
    return pfn_modify_allowed(pte_pfn(*pte),
                              *(pgprot_t*)(walk->private)) ?
             0 :
             -EACCES;
}

static int prot_none_test(unsigned long addr,
                          unsigned long next,
                          struct mm_walk* walk)
{
    return 0;
}

static const struct mm_walk_ops prot_none_walk_ops = {
    .pte_entry     = prot_none_pte_entry,
    .hugetlb_entry = prot_none_hugetlb_entry,
    .test_walk     = prot_none_test,
};

int c_mprotect_fixup(task_t* task,
                     struct vm_area_struct* vma,
                     struct vm_area_struct** pprev,
                     unsigned long start,
                     unsigned long end,
                     unsigned long newflags)
{
    struct mm_struct* mm   = vma->vm_mm;
    unsigned long oldflags = vma->vm_flags;
    long nrpages           = (end - start) >> PAGE_SHIFT;
    unsigned long charged  = 0;
    pgoff_t pgoff;
    int error;
    int dirty_accountable = 0;

    if (newflags == oldflags)
    {
        *pprev = vma;
        return 0;
    }

    /*
     * Do PROT_NONE PFN permission checks here when we can still
     * bail out without undoing a lot of state. This is a rather
     * uncommon case, so doesn't need to be very optimized.
     */
    if (arch_has_pfn_modify_check()
        && (vma->vm_flags & (VM_PFNMAP | VM_MIXEDMAP))
        && (newflags & (VM_READ | VM_WRITE | VM_EXEC)) == 0)
    {
        pgprot_t new_pgprot = vm_get_page_prot(newflags);

        error = walk_page_range(task->mm,
                                start,
                                end,
                                &prot_none_walk_ops,
                                &new_pgprot);
        if (error)
            return error;
    }

    /*
     * If we make a private mapping writable we increase our commit;
     * but (without finer accounting) cannot reduce our commit if we
     * make it unwritable again. hugetlb mapping were accounted for
     * even if read-only so there is no need to account for them here
     */
    if (newflags & VM_WRITE)
    {
        /* Check space limits when area turns into data. */
        if (!may_expand_vm(mm, newflags, nrpages)
            && may_expand_vm(mm, oldflags, nrpages))
            return -ENOMEM;
        if (!(oldflags
              & (VM_ACCOUNT | VM_WRITE | VM_HUGETLB | VM_SHARED
                 | VM_NORESERVE)))
        {
            charged = nrpages;
            if (security_vm_enough_memory_mm(mm, charged))
                return -ENOMEM;
            newflags |= VM_ACCOUNT;
        }
    }

    /*
     * First try to merge with previous and/or next vma.
     */
    pgoff  = vma->vm_pgoff + ((start - vma->vm_start) >> PAGE_SHIFT);
    *pprev = vma_merge(mm,
                       *pprev,
                       start,
                       end,
                       newflags,
                       vma->anon_vma,
                       vma->vm_file,
                       pgoff,
                       vma_policy(vma),
                       vma->vm_userfaultfd_ctx);
    if (*pprev)
    {
        vma = *pprev;
        VM_WARN_ON((vma->vm_flags ^ newflags) & ~VM_SOFTDIRTY);
        goto success;
    }

    *pprev = vma;

    if (start != vma->vm_start)
    {
        error = split_vma(mm, vma, start, 1);
        if (error)
            goto fail;
    }

    if (end != vma->vm_end)
    {
        error = split_vma(mm, vma, end, 0);
        if (error)
            goto fail;
    }

success:
    /*
     * vm_flags and vm_page_prot are protected by the mmap_sem
     * held in write mode.
     */
    vma->vm_flags     = newflags;
    dirty_accountable = vma_wants_writenotify(vma, vma->vm_page_prot);
    vma_set_page_prot(vma);

    change_protection(vma,
                      start,
                      end,
                      vma->vm_page_prot,
                      dirty_accountable,
                      0);

    /*
     * Private VM_LOCKED VMA becoming writable: trigger COW to avoid major
     * fault on access.
     */
    if ((oldflags & (VM_WRITE | VM_SHARED | VM_LOCKED)) == VM_LOCKED
        && (newflags & VM_WRITE))
    {
        populate_vma_page_range(vma, start, end, NULL);
    }

    vm_stat_account(mm, oldflags, -nrpages);
    vm_stat_account(mm, newflags, nrpages);
    // TODO: Might be not needed
    // perf_event_mmap(vma);
    return 0;

fail:
    // TODO:
    // vm_unacct_memory(charged);
    return error;
}

int _c_mprotect(task_t* task,
                uintptr_t start,
                uintptr_t size,
                int prot,
                int pkey)
{
    unsigned long nstart, end, tmp, reqprot;
    struct vm_area_struct *vma, *prev;
    int error       = -EINVAL;
    const int grows = prot & (PROT_GROWSDOWN | PROT_GROWSUP);
    const bool rier = (task->personality & READ_IMPLIES_EXEC)
                      && (prot & PROT_READ);

    start = untagged_addr(start);

    prot &= ~(PROT_GROWSDOWN | PROT_GROWSUP);
    if (grows == (PROT_GROWSDOWN | PROT_GROWSUP)) /* can't be both */
        return -EINVAL;

    if (start & ~PAGE_MASK)
        return -EINVAL;
    if (!size)
        return 0;
    size = PAGE_ALIGN(size);
    end  = start + size;
    if (end <= start)
        return -ENOMEM;
    if (!arch_validate_prot(prot, start))
        return -EINVAL;

    reqprot = prot;

    if (down_write_killable(&task->mm->mmap_sem))
        return -EINTR;

    /*
     * If userspace did not allocate the pkey, do not let
     * them use it here.
     */
    error = -EINVAL;
    if ((pkey != -1) && !mm_pkey_is_allocated(task->mm, pkey))
        goto out;

    vma   = find_vma(task->mm, start);
    error = -ENOMEM;
    if (!vma)
        goto out;
    prev = vma->vm_prev;
    if (unlikely(grows & PROT_GROWSDOWN))
    {
        if (vma->vm_start >= end)
            goto out;
        start = vma->vm_start;
        error = -EINVAL;
        if (!(vma->vm_flags & VM_GROWSDOWN))
            goto out;
    }
    else
    {
        if (vma->vm_start > start)
            goto out;

        if (unlikely(grows & PROT_GROWSUP))
        {
            end   = vma->vm_end;
            error = -EINVAL;
            if (!(vma->vm_flags & VM_GROWSUP))
                goto out;
        }
    }

    if (start > vma->vm_start)
        prev = vma;

    for (nstart = start;;)
    {
        unsigned long mask_off_old_flags;
        unsigned long newflags;
        int new_vma_pkey;

        /* Here we know that vma->vm_start <= nstart < vma->vm_end. */

        /* Does the application expect PROT_READ to imply PROT_EXEC */
        if (rier && (vma->vm_flags & VM_MAYEXEC))
            prot |= PROT_EXEC;

        /*
         * Each mprotect() call explicitly passes r/w/x permissions.
         * If a permission is not passed to mprotect(), it must be
         * cleared from the VMA.
         */
        mask_off_old_flags = VM_READ | VM_WRITE | VM_EXEC
                             | VM_FLAGS_CLEAR;

        new_vma_pkey = arch_override_mprotect_pkey(vma, prot, pkey);
        newflags     = calc_vm_prot_bits(prot, new_vma_pkey);
        newflags |= (vma->vm_flags & ~mask_off_old_flags);

        /* newflags >> 4 shift VM_MAY% in place of VM_% */

        /**
         * Let's bypass this
         */
        //         if ((newflags & ~(newflags >> 4))
        //             & (VM_READ | VM_WRITE | VM_EXEC))
        //         {
        //             error = -EACCES;
        //             goto out;
        //         }

        error = security_file_mprotect(vma, reqprot, prot);
        if (error)
            goto out;

        tmp = vma->vm_end;
        if (tmp > end)
            tmp = end;
        error = c_mprotect_fixup(task, vma, &prev, nstart, tmp, newflags);
        if (error)
            goto out;
        nstart = tmp;

        if (nstart < prev->vm_end)
            nstart = prev->vm_end;
        if (nstart >= end)
            goto out;

        vma = prev->vm_next;
        if (!vma || vma->vm_start != nstart)
        {
            error = -ENOMEM;
            goto out;
        }
        prot = reqprot;
    }
out:
    up_write(&task->mm->mmap_sem);
    return error;
}

int c_mprotect(task_t* task,
               uintptr_t start,
               uintptr_t size,
               int prot,
               int pkey)
{
    int ret;
    mm_segment_t old_fs;

    old_fs = get_fs();
    set_fs(KERNEL_DS);

    ret = _c_mprotect(task, start, size, prot, pkey);

    set_fs(old_fs);

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

unsigned long c_copy_to_user(task_t* task,
                             ptr_t to,
                             ptr_t from,
                             size_t size)
{
    mm_t* mm;
    page_t** pages;
    unsigned long result;
    mm_segment_t old_fs;
    size_t size_to_copy;
    uintptr_t real_addr, user_align_addr;
    uintptr_t shifted;
    int nr_pages, nr_page;
    uintptr_t failed_copied_bytes;
    int ret    = 0;
    int locked = 1;

    // So we can access anywhere we can in user space.
    old_fs = get_fs();
    set_fs(KERNEL_DS);

    result = size;

    if (result == 0)
    {
        goto out;
    }

    mm = get_task_mm_kthread(task);

    if (mm == NULL)
    {
        goto out;
    }

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

    nr_pages = (((size + shifted) - 1) / PAGE_SIZE) + 1;

    pages = kmalloc(nr_pages * sizeof(struct page*), GFP_KERNEL);

    down_read(&mm->mmap_sem);

    ret = pin_user_pages_remote(task,
                                mm,
                                user_align_addr,
                                nr_pages,
                                FOLL_FORCE | FOLL_WRITE,
                                pages,
                                NULL,
                                &locked);
    if (locked)
    {
        up_read(&mm->mmap_sem);
    }

    if (ret <= 0)
    {
        goto out;
    }

    for (nr_page = 0; nr_page < nr_pages; nr_page++)
    {
        real_addr = (uintptr_t)kmap(pages[nr_page]) + shifted;

        size_to_copy = PAGE_SIZE - shifted;

        if (size_to_copy > result)
        {
            size_to_copy = result;
        }

        //         c_printk_info("writing %lX-%lX with %li\n",
        //                       real_addr,
        //                       (uintptr_t)user_align_addr + shifted,
        //                       size_to_copy);

        failed_copied_bytes = copy_to_user((ptr_t)real_addr,
                                           from,
                                           size_to_copy);

        result -= (size_to_copy - failed_copied_bytes);

        if (failed_copied_bytes != 0)
        {
            goto out;
        }

        if (result <= 0)
        {
            break;
        }

        // We done the first page, we can go by copying full pages now.
        if (shifted != 0)
            shifted = 0;

        user_align_addr += PAGE_SIZE;
        *(uintptr_t*)&from += size_to_copy;
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

    unpin_user_pages_dirty_lock(pages, nr_pages, true);

out:

    if (pages)
    {
        kfree(pages);
    }

    set_fs(old_fs);

    return result;
}

unsigned long c_copy_from_user(task_t* task,
                               ptr_t to,
                               ptr_t from,
                               size_t size)
{
    mm_t* mm;
    page_t** pages = NULL;
    unsigned long result;
    mm_segment_t old_fs;
    size_t size_to_copy;
    uintptr_t real_addr, user_align_addr;
    uintptr_t shifted;
    int nr_pages, nr_page;
    uintptr_t failed_copied_bytes;
    int ret    = 0;
    int locked = 1;

    // So we can access anywhere we can in user space.
    old_fs = get_fs();
    set_fs(KERNEL_DS);

    result = size;

    if (result == 0)
    {
        goto out;
    }

    mm = get_task_mm_kthread(task);

    if (mm == NULL)
    {
        goto out;
    }

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

    nr_pages = (((size + shifted) - 1) / PAGE_SIZE) + 1;

    pages = kmalloc(nr_pages * sizeof(struct page*), GFP_KERNEL);

    down_read(&mm->mmap_sem);

    ret = pin_user_pages_remote(task,
                                mm,
                                user_align_addr,
                                nr_pages,
                                FOLL_FORCE,
                                pages,
                                NULL,
                                &locked);
    if (locked)
    {
        up_read(&mm->mmap_sem);
    }

    if (ret <= 0)
    {
        goto out;
    }

    for (nr_page = 0; nr_page < nr_pages; nr_page++)
    {
        real_addr = (uintptr_t)kmap(pages[nr_page]) + shifted;

        size_to_copy = PAGE_SIZE - shifted;

        if (size_to_copy > result)
        {
            size_to_copy = result;
        }

        //         c_printk_info("reading %lX-%lX with %li\n",
        //                       real_addr,
        //                       (uintptr_t)user_align_addr + shifted,
        //                       size_to_copy);

        failed_copied_bytes = copy_from_user(to,
                                             (ptr_t)real_addr,
                                             size_to_copy);

        result -= (size_to_copy - failed_copied_bytes);

        if (failed_copied_bytes != 0)
        {
            goto out;
        }

        if (result <= 0)
        {
            break;
        }

        // We done the first page, we can go by copying full pages now.
        if (shifted != 0)
            shifted = 0;

        user_align_addr += PAGE_SIZE;
        *(uintptr_t*)&to += size_to_copy;
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

    unpin_user_pages_dirty_lock(pages, nr_pages, false);
out:

    if (pages)
    {
        kfree(pages);
    }

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
