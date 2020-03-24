#include "communicate.h"

int init_mod(void);
void free_mod(void);

module_init(init_mod);
module_exit(free_mod);

MODULE_AUTHOR("Xutax-Kamay");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Module to \"hack\" into other processes");

int init_mod(void)
{
    // Threads should not needed anymore
	// communicate_start_thread();
    communicate_with_tasks();
	hook_kernel();

	c_printk("kernel module loaded.\n");
	return 0;
}

void free_mod(void)
{
    // Threads should not needed anymore
	// communicate_kill_thread();
    // communicate_with_tasks();
	unhook_kernel();

	c_printk("kernel module unloaded.\n");
}
