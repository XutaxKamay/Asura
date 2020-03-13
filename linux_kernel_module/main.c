#include "memutils.h"

int init_mod(void);
void free_mod(void);

int init_mod(void)
{
	c_printk("kernel module loaded.\n");
	return 0;
}

void free_mod(void)
{
	c_printk("kernel module unloaded.\n");
}

module_init(init_mod);
module_exit(free_mod);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Xutax-Kamay");
MODULE_DESCRIPTION("Module to \"hack\" into other processes");
