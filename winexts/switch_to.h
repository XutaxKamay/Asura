#ifndef SWITCH_TO_H
#define SWITCH_TO_H

extern task_t* task_attached_to;
extern task_t* current_task_switched;

void current_switch_to(task_t* task, bool show_regs);

#endif
