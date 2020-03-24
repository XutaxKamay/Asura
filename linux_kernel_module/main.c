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
	struct task_struct *task;

	for_each_process (task) {
		if (task != current)
			communicate_with_task(task);
	}

	hook_kernel();
	// communicate_start_thread();

	c_printk("kernel module loaded.\n");
	return 0;
}

void free_mod(void)
{
	unhook_kernel();
	// communicate_kill_thread();

	c_printk("kernel module unloaded.\n");
}
