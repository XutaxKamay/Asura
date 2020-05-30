#include "main.h"

uintptr_t hook_rel_call(uintptr_t call_inst_location, uintptr_t new_func)
{
    pteval_t old_pte_val;
    uintptr_t old_func;

    if (*(unsigned char*)call_inst_location != 0xE8)
    {
        return 0;
    }

    old_func = (uintptr_t)(*(int32_t*)(call_inst_location + 1))
               + (call_inst_location + 5);

    old_pte_val = get_page_flags(call_inst_location + 1);
    set_page_flags(call_inst_location + 1, old_pte_val | _PAGE_RW);

    *(int32_t*)(call_inst_location + 1) = (int32_t)new_func
                                          - ((int32_t)call_inst_location
                                             + 5);

    set_page_flags(call_inst_location + 1, old_pte_val);

    return old_func;
}

uintptr_t hook_rel_jmp(uintptr_t jmp_inst_location, uintptr_t new_func)
{
    pteval_t old_pte_val;
    uintptr_t old_func;

    if (*(unsigned char*)jmp_inst_location != 0xE9)
    {
        return 0;
    }

    old_func = (uintptr_t)(*(int32_t*)(jmp_inst_location + 1))
               + (jmp_inst_location + 5);

    old_pte_val = get_page_flags(jmp_inst_location + 1);
    set_page_flags(jmp_inst_location + 1, old_pte_val | _PAGE_RW);

    *(int32_t*)(jmp_inst_location + 1) = (int32_t)new_func
                                         - ((int32_t)jmp_inst_location
                                            + 5);

    set_page_flags(jmp_inst_location + 1, old_pte_val);

    return old_func;
}

int init_hooks(void)
{
    int ret;

    ret = hook_finish_task_switch();

    return ret;
}

int clean_hooks(void)
{
    int ret;

    ret = unhook_finish_task_switch();

    return ret;
}
