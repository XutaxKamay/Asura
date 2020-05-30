#ifndef HOOKS_H
#define HOOKS_H

uintptr_t hook_rel_call(uintptr_t call_inst_location, uintptr_t new_func);
uintptr_t hook_rel_jmp(uintptr_t jmp_inst_location, uintptr_t new_func);

int init_hooks(void);
int clean_hooks(void);

#endif
