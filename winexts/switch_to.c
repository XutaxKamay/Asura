#include "main.h"

__switch_to_t original___switch_to = NULL;

/**
 * Patterns that presents:
 * jmp __switch_to
 */
static pattern_result_t pattern_result;

/**
 * This is used to know what tasks are attached to
 */
task_t* task_attached_to[PID_MAX_LIMIT];

/**
 * We could rewrite the whole function here instead to optimize the
 * function a bit.
 * But if the kernel changes it might be not a good option..
 * For now we write twice the current_task, but this not a problem,
 * since we're inside the schedule function.
 */
__visible __notrace_funcgraph task_t* new___switch_to(task_t* prev,
                                                      task_t* next)
{
    task_t* ret;

    ret = original___switch_to(prev, next);

    if (task_attached_to[next->pid] != NULL
        && task_attached_to[next->pid] != next)
    {
        /**
         * Write the new current_task on the cpu
         */
        this_cpu_write(current_task, task_attached_to[next->pid]);
    }

    return ret;
}

int hook___switch_to(void)
{
    char patterns[][64] = { "41 5F 41 5E 41 5D 41 5C 5B 5D E9 ?? ?? ?? "
                            "?? "
                            "0F 1F 40 00" };

    /**
     * Init tasks being attached somewhere
     */
    memset(task_attached_to, 0, sizeof(task_attached_to));

    /**
     * We want only 1 call for now
     */
    init_pattern_result(&pattern_result, 0);

    if (!scan_kernel("__switch_to_asm",
                     "ret_from_fork",
                     patterns[0],
                     strlen(patterns[0]),
                     &pattern_result))
    {
        return -1;
    }

    original___switch_to = (ptr_t)kallsyms_lookup_name("__switch_to");

    if (original___switch_to == NULL)
    {
        c_printk_error("couldn't find the original __switch_to\n");
        return -1;
    }

    if (!hook_rel_jmp(pattern_result.addrs[0] + 10,
                      (uintptr_t)new___switch_to))
    {
        c_printk_error("couldn't hook __switch_to\n");
        return -1;
    }

    c_printk_info("hooked __switch_to\n");

    return 0;
}

void unhook___switch_to(void)
{
    /**
     * Shouldn't fail if we reached here
     */

    if (pattern_result.count != 0)
    {
        hook_rel_jmp(pattern_result.addrs[0] + 10,
                     (uintptr_t)original___switch_to);

        free_pattern_result(&pattern_result);

        c_printk_info("unhooked __switch_to\n");
    }
}
