#include "main.h"

bool hook_rel_call(uintptr_t call_inst_location, uintptr_t new_func)
{
    pteval_t old_pte_val;

    old_pte_val = get_page_flags(call_inst_location + 1);
    set_page_flags(call_inst_location + 1, old_pte_val | _PAGE_RW);

    if (*(unsigned char*)call_inst_location != 0xE8)
    {
        set_page_flags(call_inst_location + 1, old_pte_val);
        return false;
    }

    *(uint32_t*)(call_inst_location + 1)
      = ((uint32_t)(new_func & 0xFFFFFFFF))
        - (((uint32_t)call_inst_location + 5) & 0xFFFFFFFF);

    set_page_flags(call_inst_location + 1, old_pte_val);

    return true;
}

bool hook_rel_jmp(uintptr_t jmp_inst_location, uintptr_t new_func)
{
    pteval_t old_pte_val;

    old_pte_val = get_page_flags(jmp_inst_location + 1);
    set_page_flags(jmp_inst_location + 1, old_pte_val | _PAGE_RW);

    if (*(unsigned char*)jmp_inst_location != 0xE9)
    {
        set_page_flags(jmp_inst_location + 1, old_pte_val);
        return false;
    }

    *(uint32_t*)(jmp_inst_location + 1)
      = ((uint32_t)(new_func & 0xFFFFFFFF))
        - (((uint32_t)jmp_inst_location + 5) & 0xFFFFFFFF);

    set_page_flags(jmp_inst_location + 1, old_pte_val);

    return true;
}

int init_hooks(void)
{
    int ret;

    ret = hook___switch_to();

    return ret;
}

void clean_hooks(void)
{
    unhook___switch_to();
}
