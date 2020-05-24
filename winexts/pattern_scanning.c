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
