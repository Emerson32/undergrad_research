#include <linux/delay.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/keyboard.h>
#include <linux/module.h>
#include <linux/semaphore.h>
#include <linux/string.h>        /* memset and strncpy */
#include <linux/uaccess.h>       /* copy_to_user() */
#include <net/sock.h>            /* Needed for netlink */
#include <linux/netlink.h>
#include <linux/skbuff.h>

#define MYPROTO NETLINK_USERSOCK
#define MYGRP 31

#define SEPARATION_THRESH 80     /* Keystroke separation threshold, in ms */
#define KEYPRESS_BUFFSIZE 1024   /* Max keypress buff size */

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

/* Kernel netlink socket */
static struct sock *nl_sk = NULL;

/* Keypress data buffer */
static char keypress_buff[KEYPRESS_BUFFSIZE];

/* Default delay expressed in jiffies */
static int delay = HZ;

/* Initial time stamp for keypress calculation */
static unsigned long init_stamp = 0;

/* Caluculated time of separation between kepyresses */
static signed long stroke_separation;

/* Keypress sequence counter */
static int sequence_count = 0;

/* Object for keypress validation */
struct keypress {
    int keycode;
    char valid[1];
} key;

/* Shift key is pressed */
static int shiftKeyDepressed = 0;

/* Keyboard notification callback */
static int kb_notify(struct notifier_block *, unsigned long, void *);

/* Netlink callback */
static void nl_send_msg(void);

/* Keyboard notification callback */
static int kb_notify(struct notifier_block *nblock, unsigned long action, void *data)
{
    unsigned long jiff, curr_stamp = 0;
    jiff = jiffies;

    signed long diff;

    char keyval[21];
    char valid[2];

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
            // Acquire lock to acces global variables */
            down(&sem);

            memset(&keypress_buff, '\0', sizeof keypress_buff);

            struct keypress key = {
                .keycode = param->value,
                .valid[0] = '1'
            };

            /* Store the correct key value */
            if (shiftKeyDepressed == 0)
                strncpy(keyval, keymap[param->value], sizeof keyval);
            else
                strncpy(keyval, keymapShiftActivated[param->value], sizeof keyval);

            strncpy(keypress_buff, keyval, sizeof keyval);

            if(!init_stamp)
            {
                init_stamp = jiff;
                pr_info("init_stamp: %ld value: %d\n",
                        init_stamp, param->value);
            }
            else
            {
                /* Caluclate the keystroke time */
                curr_stamp = jiff;
                diff = (long)curr_stamp - (long)init_stamp;
                stroke_separation = diff * 1000 / delay;

                /* Adjust the init_stamp value for a future calculation*/
                init_stamp = curr_stamp;
                curr_stamp = 0;

                pr_info("stroke_separation: %ld value: %d\n",
                        stroke_separation, param->value);
            }

            // Increment the sequence count as necessary
            if (stroke_separation < SEPARATION_THRESH)
                sequence_count++;
            else
                sequence_count = 0;

            if (sequence_count == 3)
            {
                pr_alert("Suspicious typing speed detected\n");
                key.valid[0] = '0';
                sequence_count = 0;
            }

            /* Copy the validity to the valid buffer */
            strncpy(valid, key.valid, sizeof valid);

            /* Append the valid buffer to the keypress buffer */
            strncat(keypress_buff, " ", 2);
            strncat(keypress_buff, valid, 2);
            //pr_info("Keypress buffer: %s\n", keypress_buff);

            /* Send the keypress packet to user space */
            up(&sem);
            nl_send_msg();
        }
    }
    return NOTIFY_OK;
}

/* Create the notifier_block object */
static struct notifier_block kb_nb = { .notifier_call = kb_notify };

static void nl_send_msg(void)
{
    struct nlmsghdr *nlh = NULL;
    struct sk_buff *skb_out = NULL;
    int msg_size = strlen(keypress_buff + 1);
    int res;

    //pr_info("Entering message send function\n");

    //nlh = (struct nlmsghdr *)skb->data;
    //pr_info("Netlink received msg payload: %s\n",
    //        (char *)NLMSG_DATA(nlh));

    //pid = nlh->nlmsg_pid;       [> pid of sending process <]

    /* Send the keypress bufer from kernel to user */
    skb_out = nlmsg_new(msg_size, GFP_KERNEL);
    if (!skb_out)
    {
        pr_err("Failed to allocate new skb\n");
        return;
    }

    //nlh->nlmsg_len = NLMSG_SPACE(KEYPRESS_BUFFSIZE);
    //nlh->nlmsg_pid = 0;     [> from kernel <]
    //nlh->nlmsg_flags = 0;
    nlh = nlmsg_put(skb_out, 0, 1, NLMSG_DONE, msg_size, 0);

    strncpy(nlmsg_data(nlh), keypress_buff, msg_size);

    if ((res = nlmsg_multicast(nl_sk, skb_out, 0, MYGRP, GFP_KERNEL)) < 0)
    {
        pr_info("Error while sending back to user\n");
    }
}

static int __init kb_init(void)
{
    memset(&keypress_buff, '\0', sizeof keypress_buff);

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

MODULE_AUTHOR("Emerson");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Pibox keystroke monitor");
