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

#define NETLINK_USER 31
#define SEPARATION_THRESH 80     /* Keystroke separation threshold, in ms */
#define KEYPRESS_BUFFSIZE 1024   /* Max keypress buff size */

static struct semaphore sem;

/* Keypress mappings */
static const char* keymap[] = { "\0", "ESC", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=", "_BACKSPACE_", "_TAB_",
                        "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "[", "]", "_ENTER_", "_CTRL_", "a", "s", "d", "f",
                        "g", "h", "j", "k", "l", ";", "'", "`", "_SHIFT_", "\\", "z", "x", "c", "v", "b", "n", "m", ",", ".",
                        "/", "_SHIFT_", "\0", "\0", " ", "_CAPSLOCK_", "_F1_", "_F2_", "_F3_", "_F4_", "_F5_", "_F6_", "_F7_",
                        "_F8_", "_F9_", "_F10_", "_NUMLOCK_", "_SCROLLLOCK_", "_HOME_", "_UP_", "_PGUP_", "-", "_LEFT_", "5",
                        "_RTARROW_", "+", "_END_", "_DOWN_", "_PGDN_", "_INS_", "_DEL_", "\0", "\0", "\0", "_F11_", "_F12_",
                        "\0", "\0", "\0", "\0", "\0", "\0", "\0", "_ENTER_", "CTRL_", "/", "_PRTSCR_", "ALT", "\0", "_HOME_",
                        "_UP_", "_PGUP_", "_LEFT_", "_RIGHT_", "_END_", "_DOWN_", "_PGDN_", "_INSERT_", "_DEL_", "\0", "\0",
                        "\0", "\0", "\0", "\0", "\0", "_PAUSE_"};

static const char* keymapShiftActivated[] =
                        { "\0", "ESC", "!", "@", "#", "$", "%", "^", "&", "*", "(", ")", "_", "+", "_BACKSPACE_", "_TAB_",
                        "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "{", "}", "_ENTER_", "_CTRL_", "A", "S", "D", "F",
                        "G", "H", "J", "K", "L", ":", "\"", "~", "_SHIFT_", "|", "Z", "X", "C", "V", "B", "N", "M", "<", ">",
                        "?", "_SHIFT_", "\0", "\0", " ", "_CAPSLOCK_", "_F1_", "_F2_", "_F3_", "_F4_", "_F5_", "_F6_", "_F7_",
                        "_F8_", "_F9_", "_F10_", "_NUMLOCK_", "_SCROLLLOCK_", "_HOME_", "_UP_", "_PGUP_", "-", "_LEFT_", "5",
                        "_RTARROW_", "+", "_END_", "_DOWN_", "_PGDN_", "_INS_", "_DEL_", "\0", "\0", "\0", "_F11_", "_F12_",
                        "\0", "\0", "\0", "\0", "\0", "\0", "\0", "_ENTER_", "CTRL_", "/", "_PRTSCR_", "ALT", "\0", "_HOME_",
                        "_UP_", "_PGUP_", "_LEFT_", "_RIGHT_", "_END_", "_DOWN_", "_PGDN_", "_INSERT_", "_DEL_", "\0", "\0",
                        "\0", "\0", "\0", "\0", "\0", "_PAUSE_"};

/* Kernel netlink socket */
static struct sock *nl_sk = NULL;

/* Keypress data buffer */
static char keypress_buff[KEYPRESS_BUFFSIZE];

/* Default delay expressed in jiffies */
static int delay = HZ;

/* Keystroke timestamp for calculating keystroke speed */
static unsigned long init_stamp = 0;

/* Caluculated time of separation between kepyresses */
static signed long stroke_separation;

/* Sequence counter */
static int sequence_count = 0;

/* Object for keypress validation */
struct keypress {
    int keycode;
    char valid[1];
} key;

/* Shift key is pressed */
static int shiftKeyDepressed = 0;

/* Keyboard notification prototypes */
static int kb_notify(struct notifier_block *, unsigned long, void *);

/* Netlink callback */
static void nl_recv_msg(struct sk_buff *skb);

/* Netlink cfg struct */
struct netlink_kernel_cfg cfg = {
    .input = nl_recv_msg,
};

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
        if (param->value == 42 || param->value==54)
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
            // Acquire lock to read and modify the global variables
            // init_stamp and stroke_speed
            down(&sem);

            /* Reset the keypress buffer */
            memset(&keypress_buff, '\0', sizeof keypress_buff);

            struct keypress key = {
                .keycode = param->value,
                .valid = "1",
            };

            /* Copy the key literal into the keypress buffer */
            if (shiftKeyDepressed == 0)
            {
                strncpy(keyval, keymap[param->value], sizeof keyval);
            }
            else
            {
                strncpy(keyval, keymapShiftActivated[param->value], sizeof keyval);
            }

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

                // Increment the sequence count as necessary
                if (stroke_separation < SEPARATION_THRESH)
                    sequence_count++;
                else
                    sequence_count = 0;

                // Flag suspicious behavior
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
            }
            up(&sem);
        }
    }
    return NOTIFY_OK;
}

/* Create the notifier_block object */
static struct notifier_block kb_nb = { .notifier_call = kb_notify };

static void nl_recv_msg(struct sk_buff *skb)
{
    struct nlmsghdr *nlh = NULL;
    struct sk_buff *skb_out = NULL;
    int res;
    int pid;
    int msg_size = strlen(keypress_buff);

    printk(KERN_INFO "Entering message send function\n");

    nlh = (struct nlmsghdr *)skb->data;
    pr_info("Netlink received msg payload: %s\n",
            (char *)NLMSG_DATA(nlh));

    pid = nlh->nlmsg_pid;       /* pid of sending process */

    /* Send the keypress bufer from kernel to user */
    skb_out = nlmsg_new(msg_size, 0);
    if (!skb_out)
    {
        pr_err("Failed to allocate new skb\n");
        return;
    }

    //nlh->nlmsg_len = NLMSG_SPACE(KEYPRESS_BUFFSIZE);
    //nlh->nlmsg_pid = 0;     [> from kernel <]
    //nlh->nlmsg_flags = 0;
    nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size, 0);
    NETLINK_CB(skb_out).dst_group = 0;  /* not in mcast group */
    strncpy(nlmsg_data(nlh), keypress_buff, msg_size);

    res = nlmsg_unicast(nl_sk, skb_out, pid);
    if (res < 0)
    {
        pr_info("Error while sending back to user\n");
    }
    //kfree(skb_out);
}

static int __init kb_init(void)
{
    /* Fill the keypress buffer with null-bytes */
    memset(&keypress_buff, '\0', sizeof keypress_buff);

    /* Create the netlink socket */
    nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);
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
