/*
 * hello1.c - The simplest kernel module.
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Emerson");
MODULE_DESCRIPTION("Simple test driver for RPi");
MODULE_VERSION("0.1");

static char *name = "world";

// charp = char pointer, defaults to "world"
module_param(name, charp, S_IRUGO);
MODULE_PARM_DESC(name, "The name to display in /var/log/kern.log");

static int __init hello_1_init(void)
{
    printk(KERN_INFO "Hello %s from the RPi LKM\n", name);

    /*
     * A non 0 return means init_module failed; module can't be loaded.
     */
    return 0;
}

static void __exit hello_1_exit(void)
{
    printk(KERN_INFO "Goodbye %s from the RPi LKM\n", name);
}

module_init(hello_1_init);
module_exit(hello_1_exit);
