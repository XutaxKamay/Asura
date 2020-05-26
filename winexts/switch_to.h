#ifndef SWITCH_TO_H
#define SWITCH_TO_H

typedef task_t* (*__switch_to_t)(task_t*, task_t*);
extern __switch_to_t original___switch_to;

extern task_t* task_attached_to;
extern task_t* current_task_switched;

task_t* new___switch_to(task_t* prev, task_t* next);
int hook___switch_to(void);
void unhook___switch_to(void);

#endif
