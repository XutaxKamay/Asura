#include "main.h"

/**
 * This is used to know what tasks are attached to
 * We could rewrite the whole function (switch_to) here instead to
 * optimize the function a bit. But if the kernel changes it might be not
 * a good option.. For now we write twice the current_task, but this not a
 * problem, since we're inside the schedule function. There is one problem
 * with it is that spinlocks/mutexes are sometimes on wrong owner during
 * schedule. Hooking finish_task_switch is one option instead.
 */
task_t* task_attached_to      = NULL;
task_t* current_task_switched = NULL;

void current_switch_to(task_t* task, bool show_regs)
{
    if (show_regs)
        __show_regs(task_pt_regs(current), SHOW_REGS_ALL);

    /**
     * Write new current task
     */
    this_cpu_write(current_task, task);

    if (show_regs)
        __show_regs(task_pt_regs(current), SHOW_REGS_ALL);
}
