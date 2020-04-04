#include "hooks.h"

free_bprm_t original_free_bprm = NULL;
static struct mutex g_mutex;
static bool g_unhooking  = false;
static bool g_infunction = false;
pattern_result_struct_t list_calls_free_bprm;

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
    int i;
    char temp[64];

    g_unhooking = false;

    init_pattern_result(&list_calls_free_bprm, 0);

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
                      &list_calls_free_bprm);

    if (!ret)
    {
        c_printk("didn't find any signatures of calls of free_bprm\n");
        return;
    }

    for (i = 0; i < list_calls_free_bprm.count; i++)
    {
        c_printk("replacing call for free_bprm at 0x%lX\n",
                 list_calls_free_bprm.addrs[i]);

        *(ptr_t*)list_calls_free_bprm.addrs[i] = (ptr_t)new_free_bprm;
    }
}

void unhook_callsof_free_bprm(void)
{
    int i;

    g_unhooking = true;

    if (g_infunction)
    {
        mutex_lock(&g_mutex);
    }

    if (list_calls_free_bprm.count == 0)
    {
        c_printk("nothing to unhook for free_bprm\n");
        goto out;
    }

    for (i = 0; i < list_calls_free_bprm.count; i++)
    {
        c_printk("replacing call for free_bprm at 0x%lX\n",
                 list_calls_free_bprm.addrs[i]);

        *(ptr_t*)list_calls_free_bprm.addrs[i] = (ptr_t)original_free_bprm;
    }

out:
    free_pattern_result(&list_calls_free_bprm);
}
#else
void hook_callsof_free_bprm(void)
{
    int ret;
    char* pattern;
    char* pattern2;
    int i;
    pteval_t old_pte_val;

    // TODO: Might be better to convert those into an array.
    pattern = "E8 ? ? ? ? 48 89 DF E8 ? ? ? ? EB ? 41 BD F4 FF FF FF 48 "
              "8B 7D";
    pattern2 = "E8 ? ? ? ? 48 89 DF E8 ? ? ? ? 4D 85 F6 74 ? 4C 89 F7";

    g_unhooking = false;

    init_pattern_result(&list_calls_free_bprm, 0);

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
                      &list_calls_free_bprm);

    if (!ret)
    {
        c_printk("didn't find first signature of call of free_bprm\n");
    }

    ret = scan_kernel("_text",
                      "_end",
                      pattern2,
                      strlen(pattern2) - 1,
                      &list_calls_free_bprm);

    if (!ret)
    {
        c_printk("didn't find second signature of call of free_bprm\n");
    }

    if (list_calls_free_bprm.count == 0)
    {
        c_printk("didn't find any signatures of calls of free_bprm\n");
        return;
    }

    // This is safe to do as modules are always around the kernel
    // within its address space of 32 bits.
    // ((dst & 0xFFFFFFFF) - (src + 5) & 0xFFFFFFFF)

    for (i = 0; i < list_calls_free_bprm.count; i++)
    {
        old_pte_val = get_page_flags(list_calls_free_bprm.addrs[i] + 1);
        set_page_flags(list_calls_free_bprm.addrs[i] + 1,
                       old_pte_val | _PAGE_RW);

        c_printk("replacing call for free_bprm at 0x%lX\n",
                 list_calls_free_bprm.addrs[i]);

        *(uint32_t*)(list_calls_free_bprm.addrs[i] + 1)
          = ((uint32_t)((uintptr_t)new_free_bprm & 0xFFFFFFFF))
            - ((uint32_t)(list_calls_free_bprm.addrs[i] + 5)
               & 0xFFFFFFFF);

        set_page_flags(list_calls_free_bprm.addrs[i] + 1, old_pte_val);
    }
}

void unhook_callsof_free_bprm(void)
{
    int i;
    pteval_t old_pte_val;

    g_unhooking = true;

    if (g_infunction)
    {
        mutex_lock(&g_mutex);
    }

    if (list_calls_free_bprm.count == 0)
    {
        c_printk("nothing to unhook for free_bprm\n");
        goto out;
    }

    for (i = 0; i < list_calls_free_bprm.count; i++)
    {
        old_pte_val = get_page_flags(list_calls_free_bprm.addrs[i] + 1);
        set_page_flags(list_calls_free_bprm.addrs[i] + 1,
                       old_pte_val | _PAGE_RW);

        c_printk("replacing call for free_bprm at 0x%lX\n",
                 list_calls_free_bprm.addrs[i]);

        *(uint32_t*)(list_calls_free_bprm.addrs[i] + 1)
          = ((uint32_t)((uintptr_t)original_free_bprm & 0xFFFFFFFF))
            - ((uint32_t)(list_calls_free_bprm.addrs[i] + 5)
               & 0xFFFFFFFF);

        set_page_flags(list_calls_free_bprm.addrs[i] + 1, old_pte_val);
    }

out:
    free_pattern_result(&list_calls_free_bprm);
}
#endif
