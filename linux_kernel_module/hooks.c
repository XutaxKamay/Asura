#include "hooks.h"

/**
 * We could disable SMEP instead of using pte
 */
void hook_kernel(void)
{
    c_printk("hooking kernel...\n");
    hook_callsof_copy_process();
    hook_callsof_free_bprm();
    c_printk("hooked kernel\n");
}

void unhook_kernel(void)
{
    c_printk("unhooking kernel...\n");
    unhook_callsof_copy_process();
    unhook_callsof_free_bprm();
    c_printk("unhooked kernel\n");
}
