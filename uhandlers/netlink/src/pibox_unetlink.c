#include <errno.h>
#include <fcntl.h>
#include <linux/netlink.h>
#include <libudev.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

#include "bind_mgmt.h"
#include "dbg.h"
#include "id_list.h"
#include "key_packet.h"

#define MAX_PAYLOAD  1024

#define MAX_SEQ        4
#define MAX_DEVBUFF    1024
#define MAX_IDBUFF     12

#define MAX_PIPE_BUFF 8
#define ATTACH_PIPE_PATH   "/tmp/pibox_attach_pipe"
#define ALERT_PIPE_PATH    "/tmp/pibox_alert_pipe"

static struct ID_List *id_list; 
static pthread_mutex_t idl_mtx = PTHREAD_MUTEX_INITIALIZER;  /* For ID_List */

/*
* Iterates over input devices within /sys/clas/input and
* stores the bus IDs of each device with the name eventX
*/
void acquire_bus_ids(
    struct udev *udev_ctx,
    struct udev_list_entry *devs,
    struct ID_List *list)
{
    int ret;

    struct udev_device *device;
    struct udev_list_entry *dlist_entry;    /* current device list entry */

    /* Iterate over the device list and store the respective bus IDs */
    udev_list_entry_foreach(dlist_entry, devs)
    {
        char *curr_id = malloc(MAX_IDBUFF);
        char *curr_devpath = malloc(MAX_DEVBUFF);

        const char *path;
        const char *dev_name;

        // Acquire a device handle through a syspath, e.g. /sys/input/event0
        path = udev_list_entry_get_name(dlist_entry);
        device = udev_device_new_from_syspath(udev_ctx, path);

        dev_name = udev_device_get_sysname(device);

        /* Only worry about eventX device names */
        if (strstr(dev_name, "event"))
        {
            debug("I: DEVNAME=%s", udev_device_get_sysname(device));

            memset(curr_id, '\0', MAX_IDBUFF);
            memset(curr_devpath, '\0', MAX_DEVBUFF);

            // Get the current device path
            strncat(
                curr_devpath, udev_device_get_devpath(device),
                MAX_DEVBUFF - 1);

            ret = find_bus_id(curr_id, curr_devpath);
            if (ret < 0)
            {
                debug("Failed to parse the bus id for %s", dev_name);
                continue;
            }

            debug("Found bus_id %s for %s\n", curr_id, dev_name);
            id_list_add(list, &curr_id);
        }
        udev_device_unref(device);
    }
}

static void *reattach_listener(void *pid)
{
    int attach_fd;
    int rv;
    FILE *bind_fp;
    ssize_t bytes_read;
    char buff[MAX_PIPE_BUFF];

    while (1)
    {
        attach_fd = open(ATTACH_PIPE_PATH, O_RDONLY);
        if (attach_fd < 0)
        {
            log_err("Failed to obtain fd for attach pipe");
            return NULL;
        }

        bytes_read = read(attach_fd, buff, MAX_PIPE_BUFF);
        if (bytes_read < 0)
        {
            log_err("Failed to read attach pipe");
        }

        bind_fp = fopen(BIND_PATH, "w");
        if (!bind_fp)
        {
            log_err("Failed to open BIND_PATH for writing");
            return NULL;
        }

        if (strncmp(buff, "Reattach", MAX_PIPE_BUFF) == 0)
        {
            rv = pthread_mutex_lock(&idl_mtx);
            if (rv != 0)
            {
                perror("pthread_mutex_lock");
                return NULL;
            }

            for (int i = 0; i < id_list_size(id_list); i++)
            {
                char *curr_id = id_list_get(id_list, i);
                if (write_to_sys(&bind_fp, &curr_id) < 0)
                {
                    debug("Failed to write %s to %s",
                        curr_id, BIND_PATH
                    );
                }
            }

            id_list_clear(id_list);

            rv = pthread_mutex_unlock(&idl_mtx);
            if (rv != 0)
            {
                perror("pthread_mutex_unlock");
                return NULL;
            }
        }

        fclose(bind_fp);
        close(attach_fd);
    }

    return NULL;
}

int spawn_gui()
{
    pid_t pid = fork();

    if (pid < 0)
        return -1;
    
    if (pid == 0)
    {
        char *gui_path = "/home/pi/dev_files/pibox/pibox_gui.py";

        /* Execute gui program */
        if (execlp("python3", "python3", gui_path, (char *)NULL) < 0)
        {
            perror("execlp");
            return -1;
        }
    }
    return pid;
}

int main()
{
    int nls;
    int valid;
    int rv;
    char packet[MAX_PAYLOAD];

    FILE *unbind_fp;

    unbind_fp = fopen(UNBIND_PATH, "w");
    if (!unbind_fp)
    {
        log_err("Failed to open UNBIND_PATH for writing");
        return -1;
    }
    
    id_list = id_list_create();

    rv = mkfifo(ATTACH_PIPE_PATH, 0666);
    if (rv < 0)
    {
        if (errno == EEXIST)
        {
            log_info("Attach pipe already exists");
        }
        else
        {
            log_err("Failed to make pipe for attach events");
            return -1;
        }
    }

    rv = mkfifo(ALERT_PIPE_PATH, 0666);
    if (rv < 0)
    {
        if (errno == EEXIST)
        {
            log_info("Alert pipe already exists");
        }
        else
        {
            log_err("Failed to make pipe for alert events");
            return -1;
        }
    }

    pid_t gui_pid = spawn_gui();
    if (gui_pid < 0)
    {
        log_err("Failed to spawn gui process");
        return -1;
    }

    pthread_t listener_tid;
    rv = pthread_create(&listener_tid, NULL, reattach_listener, &gui_pid);
    if (rv != 0)
    {
        log_err("Failed to create listener thread for reattach events");
        return -1;
    }

    rv = pthread_detach(listener_tid);     /* Avoid zombie threads */
    if (rv != 0)
    {
        log_err("Failed to detach listener thread");
        return -1;
    }

    int is_first_stamp = 1;
    int sequence_count = 0;
    unsigned long stroke_separation = 0;
    long separation_sum = 0;
    double avg_separation = 0;

    if ((nls = open_netlink()) < 0)
        return nls;

    while (1)
    {
        valid = 1;

        /* Receive the keystroke packet */
        memset(&packet, '\0', sizeof packet);
        rv = recv_packet(nls, packet);
        if (rv < 0)
        {
            log_err("[!] Failed to read packet");
        }

        debug();
        debug("Packet contents: %s", packet);
        debug("Sequence count: %d", sequence_count);

        /* Do not add the very first stamp to the total */
        if (is_first_stamp)
        {
            sequence_count++;
            is_first_stamp = 0;
            continue;
        }

        stroke_separation = get_separation(packet);
        if (stroke_separation == -1)
        {
            log_err("[!] Failed to parse separation measurement");
            continue;
        }

        separation_sum += stroke_separation;
        sequence_count++;

        /* Evaluate the validity after four keypresses */
        if (sequence_count == MAX_SEQ)
        {
            // Calculate avg of 3 separation measurements
            avg_separation = (double)separation_sum / (MAX_SEQ - 1);
            debug("Average key separation: %.2f", avg_separation);
            valid = validate_packet(&avg_separation);

            separation_sum = 0;
            avg_separation = 0;
            sequence_count = 0;
        }
        if (valid)
        {
            debug("Valid packet found: %s", packet);
        }
        else
        {
            debug("[+] Invalid key sequence detected\n");

            struct udev *udev;
            struct udev_enumerate *enumerate;
            struct udev_list_entry *devices;

            udev = udev_new();
            if (!udev)
            {
                debug("Failed to create udev context.");
                return -1;
            }

            enumerate = udev_enumerate_new(udev);
            if (!enumerate)
            {
                debug("Failed to create enumerate context.");
                udev_unref(udev);
                return -1;
            }

            /* Specify subsystem as input and scan for devices */
            udev_enumerate_add_match_subsystem(enumerate, "input");

            /* Only find devices with the property ID_INPUT_KEYBOARD */
            udev_enumerate_add_match_property(
                enumerate,
                "ID_INPUT_KEYBOARD", "1");

            udev_enumerate_scan_devices(enumerate);

            /* Populate the device list */
            devices = udev_enumerate_get_list_entry(enumerate);
            if (!devices)
            {
                debug("Failed to populate the device list");
                goto cleanup;
            }

            /* Iterate over the device list and store the respective bus IDs */
            acquire_bus_ids(udev, devices, id_list);

            /* Unbind all devices in the device list */
            rv = pthread_mutex_lock(&idl_mtx);
            if (rv != 0)
            {
                perror("pthread_mutex_lock");
                return -1;
            }

            for (int i = 0; i < id_list_size(id_list); i++)
            {
                char *curr_id = id_list_get(id_list, i);
                if (write_to_sys(&unbind_fp, &curr_id) < 0)
                {
                    debug("Failed to write %s to %s",
                        curr_id, UNBIND_PATH
                    );
                }
            }

            rv = pthread_mutex_unlock(&idl_mtx);
            if (rv != 0)
            {
                perror("pthread_mutex_unlock");
                return -1;
            }

            cleanup:
            udev_enumerate_unref(enumerate);
            udev_unref(udev);
        }
    }
    id_list_destroy(id_list);
    fclose(unbind_fp);
    close(nls);

    return 0;
}