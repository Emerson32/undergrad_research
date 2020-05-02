/*
 * hello1.c - The simplest kernel module.
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

static int __init hello_1_init(void)
{
    printk(KERN_INFO "Hello world 1.\n");

    /*
     * A non 0 return means init_module failed; module can't be loaded.
     */
    return 0;
}

static void __exit hello_1_exit(void)
{
    printk(KERN_INFO "Goodbye world 1.\n");
}

module_init(hello_1_init);
module_exit(hello_1_exit);
