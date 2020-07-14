#include "main.h"

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
