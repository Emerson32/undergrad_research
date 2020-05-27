/*
 * pi_box_usb.c - USB driver for the pibox system
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/usb.h>

#define USB_VENDOR_ID 0x0781
#define USB_PRODUCT_ID 0x5575

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Emerson");
MODULE_DESCRIPTION("Main usb driver for pibox");
MODULE_VERSION("0.1");

/* Define the table of devices that work with this driver */
static struct usb_device_id pbusb_table[] = {
    { USB_DEVICE(USB_VENDOR_ID, USB_PRODUCT_ID) },
    { }                             /* Terminating entry */
};
MODULE_DEVICE_TABLE(usb, pbusb_table);

/* Get a minor range for your devices from the usb maintainer */
//#define USB_PBUSB_MINOR_BASE 192

static struct usb_driver pbusb_driver;

static int pbusb_probe(struct usb_interface *intf,
                      const struct usb_device_id *id)
{
    printk(KERN_INFO "[*] SanDisk device (%04X, %04X) is plugged in\n",
            id->idVendor, id->idProduct);

    return 0;
}

static void pbusb_disconnect(struct usb_interface *intf)
{
    printk(KERN_INFO "[*] SanDisk device (%04X, %04X) unplugged",
            USB_VENDOR_ID, USB_PRODUCT_ID);
}

static struct usb_driver pbusb_driver = {
    .name = "pibox_usb",
    .id_table = pbusb_table,
    .probe = pbusb_probe,
    .disconnect = pbusb_disconnect,
};

static int __init pibox_usb_init(void)
{
    printk(KERN_INFO "Loaded pibox usb module\n");

    /* Register this driver with the USB subsystem */
    int result = usb_register(&pbusb_driver);
    if (result)
        pr_err("usb_register failed! Error number %d\n", result);

    return result;
}

static void __exit pibox_usb_exit(void)
{
    /* Deregister this driver with the USB subsystem */
    usb_deregister(&pbusb_driver);
}

module_init(pibox_usb_init);
module_exit(pibox_usb_exit);
