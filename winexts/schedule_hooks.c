#include "main.h"

finish_task_switch_t original_finish_task_switch = NULL;

static pattern_result_t pattern_result_finish_task_switch;

void new_finish_task_switch(task_t* prev)
{
    original_finish_task_switch(prev);
}

int hook_finish_task_switch(void)
{
    char* pattern = "E8 ?? ?? ?? ?? EB 69 48 8B 05 ?? ?? ?? ?? 48 89 DF";

    init_pattern_result(&pattern_result_finish_task_switch, 0);

    if (!scan_kernel("__schedule",
                     "schedule",
                     pattern,
                     strlen(pattern) + 1,
                     &pattern_result_finish_task_switch))
    {
        c_printk_error("couldn't find pattern for finish_task_switch\n");
        free_pattern_result(&pattern_result_finish_task_switch);
        return -1;
    }

    /**
     * We want only one in this context (function inside context_switch
     * but compiler inlines it which results into __schedule function)
     */
    if (pattern_result_finish_task_switch.count != 1)
    {
        c_printk_error("pattern result for finish_task_switch should "
                       "have only one reference\n");
        free_pattern_result(&pattern_result_finish_task_switch);
        return -1;
    }

    if ((original_finish_task_switch = (ptr_t)hook_rel_call(
           pattern_result_finish_task_switch.addrs[0],
           (uintptr_t)new_finish_task_switch))
        == NULL)
    {
        c_printk_error("couldn't hook finish_task_switch\n");
        free_pattern_result(&pattern_result_finish_task_switch);
        return -1;
    }

    c_printk_info("hooked finish_task_switch (original func: %lX)\n",
                  (uintptr_t)original_finish_task_switch);

    return 0;
}

int unhook_finish_task_switch(void)
{
    if (pattern_result_finish_task_switch.count > 0)
    {
        if (!hook_rel_call(pattern_result_finish_task_switch.addrs[0],
                           (uintptr_t)original_finish_task_switch))
        {
            c_printk_error("couldn't unhook finish_task_switch\n");
            free_pattern_result(&pattern_result_finish_task_switch);
            return -1;
        }
    }

    free_pattern_result(&pattern_result_finish_task_switch);

    c_printk_info("unhooked finish_task_switch\n");

    return 0;
}
