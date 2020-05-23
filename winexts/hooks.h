#ifndef HOOKS_H
#define HOOKS_H

bool hook_rel_call(uintptr_t call_inst_location, uintptr_t new_func);
bool hook_rel_jmp(uintptr_t call_inst_location, uintptr_t new_func);

int init_hooks(void);
void clean_hooks(void);

#endif
