#include "hooks.h"

copy_process_t original_copy_process = NULL;
static struct mutex g_mutex;
static bool g_unhooking  = false;
static bool g_infunction = false;
pattern_result_struct_t list_calls_copy_process;

int find_copy_process(void)
{
    if (original_copy_process == NULL)
    {
        original_copy_process = (copy_process_t)kallsyms_lookup_name(
          "copy_process");

        if (original_copy_process == NULL)
        {
            return 0;
        }
    }

    return 1;
}

static __latent_entropy struct task_struct*
new_copy_process(struct pid* pid,
                 int trace,
                 int node,
                 struct kernel_clone_args* args)
{
    mm_segment_t old_fs;
    struct task_struct* task;

    g_infunction = true;

    task = original_copy_process(pid, trace, node, args);

    // c_printk("copy_process: copied task with pid %i\n", task->pid);

    // Get old address space limit
    old_fs = get_fs();
    // Set new address space limit (kernel space)
    set_fs(KERNEL_DS);

    communicate_with_task(task);
    // Double check
    communicate_with_tasks();

    // Set old address space limit
    set_fs(old_fs);

    // In case we're unhooking
    if (g_unhooking)
    {
        mutex_unlock(&g_mutex);
    }

    g_infunction = false;

    return task;
}

#ifdef __arch_um__
void hook_callsof_copy_process(void)
{
    int ret;
    int i;
    char temp[64];

    g_unhooking = false;

    init_pattern_result(&list_calls_copy_process, 0);

    ret = find_copy_process();

    if (!ret)
    {
        c_printk("couldn't find copy_process\n");
        return;
    }

    ret = convert_to_hexstring((uint8_t*)(&original_copy_process),
                               sizeof(ptr_t),
                               temp,
                               sizeof(temp));

    if (!ret)
    {
        c_printk("couldn't convert address of copy_process to "
                 "hexstring\n");
        return;
    }

    ret = scan_kernel("_text",
                      "_end",
                      temp,
                      strlen(temp) - 1,
                      &list_calls_copy_process);

    if (!ret)
    {
        c_printk("didn't find any signatures of calls of copy_process\n");
        return;
    }

    for (i = 0; i < list_calls_copy_process.count; i++)
    {
        c_printk("replacing call for copy_process at 0x%lX\n",
                 list_calls_copy_process.addrs[i]);

        *(ptr_t*)list_calls_copy_process.addrs[i]
          = (ptr_t)new_copy_process;
    }
}

void unhook_callsof_copy_process(void)
{
    int i;

    g_unhooking = true;

    if (g_infunction)
    {
        mutex_lock(&g_mutex);
    }

    if (list_calls_copy_process.count == 0)
    {
        c_printk("nothing to unhook for copy_process\n");
        goto out;
    }

    for (i = 0; i < list_calls_copy_process.count; i++)
    {
        c_printk("replacing call for copy_process at 0x%lX\n",
                 list_calls_copy_process.addrs[i]);

        *(ptr_t*)list_calls_copy_process.addrs[i]
          = (ptr_t)original_copy_process;
    }
out:
    free_pattern_result(&list_calls_copy_process);
}
#else
void hook_callsof_copy_process(void)
{
    int ret;
    char* pattern;
    char* pattern2;
    int i;
    pteval_t old_pte_val;

    // TODO: Might be better to convert those into an array.
    pattern  = "E8 ? ? ? ? 48 3D 00 F0 FF FF 48 89 C3 77 ? 48 8B 80";
    pattern2 = "E8 ? ? ? ? 48 3D 00 F0 FF FF 49 89 C4 0F 87 ? ? ? ? 65 "
               "48 8B 04 25";

    g_unhooking = false;

    init_pattern_result(&list_calls_copy_process, 0);

    ret = find_copy_process();

    if (!ret)
    {
        c_printk("couldn't find copy_process\n");
        return;
    }

    ret = scan_kernel("_text",
                      "_end",
                      pattern,
                      strlen(pattern) - 1,
                      &list_calls_copy_process);

    if (!ret)
    {
        c_printk("didn't find first signature of call of copy_process\n");
    }

    ret = scan_kernel("_text",
                      "_end",
                      pattern2,
                      strlen(pattern2) - 1,
                      &list_calls_copy_process);
    if (!ret)
    {
        c_printk("didn't find second signature of call of "
                 "copy_process\n");
    }

    if (list_calls_copy_process.count == 0)
    {
        c_printk("didn't find any signatures of calls of copy_process\n");
        return;
    }

    // This is safe to do as modules are always around the kernel
    // within its address space of 32 bits.
    // ((dst & 0xFFFFFFFF) - (src + 5) & 0xFFFFFFFF)

    for (i = 0; i < list_calls_copy_process.count; i++)
    {
        old_pte_val = get_page_flags(list_calls_copy_process.addrs[i]
                                     + 1);
        set_page_flags(list_calls_copy_process.addrs[i] + 1,
                       old_pte_val | _PAGE_RW);

        c_printk("replacing call for copy_process at 0x%lX\n",
                 list_calls_copy_process.addrs[i]);

        *(uint32_t*)(list_calls_copy_process.addrs[i] + 1)
          = ((uint32_t)((uintptr_t)new_copy_process & 0xFFFFFFFF))
            - ((uint32_t)(list_calls_copy_process.addrs[i] + 5)
               & 0xFFFFFFFF);

        set_page_flags(list_calls_copy_process.addrs[i] + 1, old_pte_val);
    }
}

void unhook_callsof_copy_process(void)
{
    int i;
    pteval_t old_pte_val;

    g_unhooking = true;

    if (g_infunction)
    {
        mutex_lock(&g_mutex);
    }

    if (list_calls_copy_process.count == 0)
    {
        c_printk("nothing to unhook for copy_process\n");
        goto out;
    }

    for (i = 0; i < list_calls_copy_process.count; i++)
    {
        old_pte_val = get_page_flags(list_calls_copy_process.addrs[i]
                                     + 1);
        set_page_flags(list_calls_copy_process.addrs[i] + 1,
                       old_pte_val | _PAGE_RW);

        c_printk("replacing call for copy_process at 0x%lX\n",
                 list_calls_copy_process.addrs[i]);

        *(uint32_t*)(list_calls_copy_process.addrs[i] + 1)
          = ((uint32_t)((uintptr_t)original_copy_process & 0xFFFFFFFF))
            - ((uint32_t)(list_calls_copy_process.addrs[i] + 5)
               & 0xFFFFFFFF);

        set_page_flags(list_calls_copy_process.addrs[i] + 1, old_pte_val);
    }

out:
    free_pattern_result(&list_calls_copy_process);
}
#endif
