#include "hooks.h"

copy_process_t original_copy_process = NULL;
struct buffer_struct buffer_list_calls_copy_process;
static struct mutex g_mutex;
static bool g_unhooking  = false;
static bool g_infunction = false;

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

    // c_printk("copy_process: created task with pid %i\n", task->pid);

    // Get old address space limit
    old_fs = get_fs();
    // Set new address space limit (kernel space)
    set_fs(KERNEL_DS);

    communicate_with_task(task);
    communicate_check_tasks();

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

void hook_callsof_copy_process(void)
{
    int ret;
    int count_calls;
    int i;
    char temp[64];
#ifdef BIT_64
    uint16_t movabs_ax;
#else
    uint8_t movabs_ax;
#endif
    uintptr_t* list_calls;

    g_unhooking = false;

    init_buffer(&buffer_list_calls_copy_process);

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
                      &buffer_list_calls_copy_process);

    if (!ret)
    {
        c_printk("didn't find any signatures of calls of copy_process\n");
        return;
    }

    count_calls = buffer_list_calls_copy_process.size / sizeof(ptr_t);

#ifdef BIT_64
    list_calls = (uintptr_t*)buffer_list_calls_copy_process.addr;

    for (i = 0; i < count_calls; i++)
    {
        movabs_ax = *(uint16_t*)(list_calls[i] - 2);

        if (movabs_ax == 0xb848)
        {
            c_printk("removing call for copy_process at 0x%p\n",
                     (ptr_t)list_calls[i]);
            *(ptr_t*)list_calls[i] = (ptr_t)new_copy_process;
        }
    }
#else
    for (i = 0; i < count_calls; i++)
    {
        movabs_ax = *(uint16_t*)(list_calls[i] - 1);

        if (movabs_ax == 0xb8)
        {
            c_printk("removing call for copy_process at 0x%p\n",
                     (ptr_t)list_calls[i]);
            *(ptr_t*)list_calls[i] = (ptr_t)new_copy_process;
        }
    }
#endif
}

void unhook_callsof_copy_process(void)
{
    int count_calls;
    int i;
#ifdef BIT_64
    uint16_t movabs_ax;
#else
    uint8_t movabs_ax;
#endif
    uintptr_t* list_calls;

    g_unhooking = true;

    if (g_infunction)
    {
        mutex_lock(&g_mutex);
    }

    if (buffer_list_calls_copy_process.addr == NULL)
    {
        c_printk("nothing to unhook for copy_process\n");
        goto out;
    }

    count_calls = buffer_list_calls_copy_process.size / sizeof(ptr_t);

#ifdef BIT_64
    list_calls = (uintptr_t*)buffer_list_calls_copy_process.addr;

    for (i = 0; i < count_calls; i++)
    {
        movabs_ax = *(uint16_t*)(list_calls[i] - 2);

        if (movabs_ax == 0xb848)
        {
            c_printk("removing call for copy_process at 0x%p\n",
                     (ptr_t)list_calls[i]);
            *(ptr_t*)list_calls[i] = original_copy_process;
        }
    }
#else
    for (i = 0; i < count_calls; i++)
    {
        movabs_ax = *(uint16_t*)(list_calls[i] - 1);

        if (movabs_ax == 0xb8)
        {
            c_printk("removing call for copy_process at 0x%p\n",
                     (ptr_t)list_calls[i]);
            *(ptr_t*)list_calls[i] = original_copy_process;
        }
    }
#endif
out:
    free_buffer(&buffer_list_calls_copy_process);
}
