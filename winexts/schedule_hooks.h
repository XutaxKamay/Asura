#ifndef SCHEDULE_HOOKS_H
#define SCHEDULE_HOOKS_H

typedef void(*finish_task_switch_t)(task_t*);

extern finish_task_switch_t original_finish_task_switch;

void new_finish_task_switch(task_t* prev);

int hook_finish_task_switch(void);
int unhook_finish_task_switch(void);

#endif
