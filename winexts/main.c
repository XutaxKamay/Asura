#include "main.h"

static dev_t g_dev;
static struct cdev g_cdev;
static struct class* g_cl;

int init_mod(void);
void free_mod(void);

module_init(init_mod);
module_exit(free_mod);

MODULE_AUTHOR("Xutax-Kamay");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Module that brings some windows API functions");

int init_mod(void)
{
    int ret = alloc_chrdev_region(&g_dev, 0, 1, DEVICE_FILE_NAME);

    if (ret < 0)
    {
        c_printk_error("failed to register device %s\n",
                       DEVICE_FILE_NAME);
        return ret;
    }

    c_printk_info("registered device %s\n", DEVICE_FILE_NAME);

    g_cl = class_create(THIS_MODULE, DEVICE_CLASS_NAME);

    if (g_cl == NULL)
    {
        unregister_chrdev_region(g_dev, 1);

        c_printk_error("failed to create class name %s for device %s\n",
                       DEVICE_FILE_NAME,
                       DEVICE_CLASS_NAME);
        return -1;
    }

    cdev_init(&g_cdev, &g_fops);

    if (cdev_add(&g_cdev, g_dev, 1) < 0)
    {
        class_destroy(g_cl);
        unregister_chrdev_region(g_dev, 1);
        c_printk_error("device %s addition failed\n", DEVICE_FILE_NAME);
        return -1;
    }

    c_printk_info("created class name %s for device %s\n",
                  DEVICE_FILE_NAME,
                  DEVICE_CLASS_NAME);

    if (device_create(g_cl, NULL, g_dev, NULL, DEVICE_FMT) == NULL)
    {
        class_destroy(g_cl);
        cdev_del(&g_cdev);
        unregister_chrdev_region(g_dev, 1);

        c_printk_error("failed to create device %s\n", DEVICE_FILE_NAME);
        return -1;
    }

    c_printk_info("successfully created device %s\n", DEVICE_FILE_NAME);

    if (find_css_set_lock() < 0)
    {
        c_printk_info("couldn't find css_set_lock\n");
        return -1;
    }

    /*
    c_printk_info("found css_set_lock at %lX\n",
                  (uintptr_t)pcss_set_lock);

    if (find_tasklist_lock() < 0)
    {
        c_printk_info("couldn't find find_tasklist_lock\n");
        return -1;
    }

    c_printk_info("found find_tasklist_lock at %lX\n",
                  (uintptr_t)ptasklist_lock);
    */

    c_printk("kernel module loaded.\n");

    return 0;
}

void free_mod(void)
{
    device_destroy(g_cl, g_dev);
    class_destroy(g_cl);
    cdev_del(&g_cdev);
    unregister_chrdev_region(g_dev, 1);

    c_printk("kernel module unloaded.\n");
}
