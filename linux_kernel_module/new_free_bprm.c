#include "hooks.h"

free_bprm_t original_free_bprm = NULL;
static struct mutex g_mutex;
static bool g_unhooking  = false;
static bool g_infunction = false;
struct buffer_struct buffer_list_calls_free_bprm;

int find_free_bprm(void)
{
    if (original_free_bprm == NULL)
    {
        original_free_bprm = (free_bprm_t)kallsyms_lookup_name("free_"
                                                               "bprm");

        if (original_free_bprm == NULL)
        {
            return 0;
        }
    }

    return 1;
}

static int new_free_bprm(struct linux_binprm* bprm)
{
    mm_segment_t old_fs;
    int result;

    g_infunction = true;

    // c_printk("free_bprm: created task with pid %i\n", current->pid);

    // Get old address space limit
    old_fs = get_fs();
    // Set new address space limit (kernel space)
    set_fs(KERNEL_DS);

    communicate_with_task(current);
    // Double check
    communicate_with_tasks();

    // Set old address space limit
    set_fs(old_fs);

    result = original_free_bprm(bprm);

    if (g_unhooking)
    {
        mutex_unlock(&g_mutex);
    }

    g_infunction = false;

    return result;
}

#ifdef __arch_um__
void hook_callsof_free_bprm(void)
{
    int ret;
    int count_calls;
    int i;
    char temp[64];
    uintptr_t* list_calls;

    g_unhooking = false;

    init_buffer(&buffer_list_calls_free_bprm);

    ret = find_free_bprm();

    if (!ret)
    {
        c_printk("couldn't find free_bprm\n");
        return;
    }

    ret = convert_to_hexstring((uint8_t*)(&original_free_bprm),
                               sizeof(ptr_t),
                               temp,
                               sizeof(temp));

    if (!ret)
    {
        c_printk("couldn't convert address of free_bprm to "
                 "hexstring\n");
        return;
    }

    ret = scan_kernel("_text",
                      "_end",
                      temp,
                      strlen(temp) - 1,
                      &buffer_list_calls_free_bprm);

    if (!ret)
    {
        c_printk("didn't find any signatures of calls of free_bprm\n");
        return;
    }

    count_calls = buffer_list_calls_free_bprm.size / sizeof(ptr_t);

    list_calls = (uintptr_t*)buffer_list_calls_free_bprm.addr;

    for (i = 0; i < count_calls; i++)
    {
        c_printk("replacing call for free_bprm at 0x%lX\n",
                 list_calls[i]);

        *(ptr_t*)list_calls[i] = (ptr_t)new_free_bprm;
    }

    for (i = 0; i < count_calls; i++)
    {
        c_printk("replacing call for free_bprm at 0x%lX\n",
                 list_calls[i]);

        *(ptr_t*)list_calls[i] = (ptr_t)new_free_bprm;
    }
}

void unhook_callsof_free_bprm(void)
{
    int count_calls;
    int i;
    uintptr_t* list_calls;

    g_unhooking = true;

    if (g_infunction)
    {
        mutex_lock(&g_mutex);
    }

    if (buffer_list_calls_free_bprm.addr == NULL)
    {
        c_printk("nothing to unhook for free_bprm\n");
        goto out;
    }

    count_calls = buffer_list_calls_free_bprm.size / sizeof(ptr_t);

    list_calls = (uintptr_t*)buffer_list_calls_free_bprm.addr;

    for (i = 0; i < count_calls; i++)
    {
        c_printk("replacing call for free_bprm at 0x%lX\n",
                 list_calls[i]);

        *(ptr_t*)list_calls[i] = (ptr_t)original_free_bprm;
    }

out:
    free_buffer(&buffer_list_calls_free_bprm);
}
#else
void hook_callsof_free_bprm(void)
{
    int ret;
    char* pattern;
    char* pattern2;
    uintptr_t* list_calls;
    int count_calls;
    int i;
    pteval_t old_pte_val;

    // TODO: Might be better to convert those into an array.
    pattern = "E8 ? ? ? ? 48 89 DF E8 ? ? ? ? EB ? 41 BD F4 FF FF FF 48 "
              "8B 7D";
    pattern2 = "E8 ? ? ? ? 48 89 DF E8 ? ? ? ? 4D 85 F6 74 ? 4C 89 F7";

    g_unhooking = false;

    init_buffer(&buffer_list_calls_free_bprm);

    ret = find_free_bprm();

    if (!ret)
    {
        c_printk("couldn't find free_bprm\n");
        return;
    }

    ret = scan_kernel("_text",
                      "_end",
                      pattern,
                      strlen(pattern) - 1,
                      &buffer_list_calls_free_bprm);

    if (!ret)
    {
        c_printk("didn't find first signature of call of free_bprm\n");
    }

    ret = scan_kernel("_text",
                      "_end",
                      pattern2,
                      strlen(pattern2) - 1,
                      &buffer_list_calls_free_bprm);

    if (!ret)
    {
        c_printk("didn't find second signature of call of free_bprm\n");
    }

    if (buffer_list_calls_free_bprm.addr == NULL)
    {
        c_printk("didn't find any signatures of calls of free_bprm\n");
        return;
    }

    count_calls = buffer_list_calls_free_bprm.size / sizeof(ptr_t);

    list_calls = (uintptr_t*)buffer_list_calls_free_bprm.addr;

    // This is safe to do as modules are always around the kernel
    // within its address space of 32 bits.
    // ((dst & 0xFFFFFFFF) - (src + 5) & 0xFFFFFFFF)

    for (i = 0; i < count_calls; i++)
    {
        old_pte_val = get_page_flags(list_calls[i] + 1);
        set_page_flags(list_calls[i] + 1, old_pte_val | _PAGE_RW);

        c_printk("replacing call for free_bprm at 0x%lX\n",
                 list_calls[i]);

        *(uint32_t*)(list_calls[i] + 1)
          = ((uint32_t)((uintptr_t)new_free_bprm & 0xFFFFFFFF))
            - ((uint32_t)(list_calls[i] + 5) & 0xFFFFFFFF);

        set_page_flags(list_calls[i] + 1, old_pte_val);
    }
}

void unhook_callsof_free_bprm(void)
{
    uintptr_t* list_calls;
    int count_calls;
    int i;
    pteval_t old_pte_val;

    g_unhooking = true;

    if (g_infunction)
    {
        mutex_lock(&g_mutex);
    }

    if (buffer_list_calls_free_bprm.addr == NULL)
    {
        c_printk("nothing to unhook for free_bprm\n");
        goto out;
    }

    count_calls = buffer_list_calls_free_bprm.size / sizeof(ptr_t);

    list_calls = (uintptr_t*)buffer_list_calls_free_bprm.addr;

    for (i = 0; i < count_calls; i++)
    {
        old_pte_val = get_page_flags(list_calls[i] + 1);
        set_page_flags(list_calls[i] + 1, old_pte_val | _PAGE_RW);

        c_printk("replacing call for free_bprm at 0x%lX\n",
                 list_calls[i]);

        *(uint32_t*)(list_calls[i] + 1)
          = ((uint32_t)((uintptr_t)original_free_bprm & 0xFFFFFFFF))
            - ((uint32_t)(list_calls[i] + 5) & 0xFFFFFFFF);

        set_page_flags(list_calls[i] + 1, old_pte_val);
    }

out:
    free_buffer(&buffer_list_calls_free_bprm);
}
#endif
