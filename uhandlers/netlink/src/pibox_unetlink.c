/*
 * User-space netlink listener
 */

#include <errno.h>
#include <linux/netlink.h>
#include <libudev.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "bind_mgmt.h"
#include "dbg.h"
#include "id_list.h"
#include "key_packet.h"

#define MYPROTO      NETLINK_USERSOCK
#define MYGRP        31
#define MAX_PAYLOAD  1024

#define MAX_SEQ             4

int open_netlink(void)
{
    int sock_fd;
    struct sockaddr_nl addr;
    int group = MYGRP;

    if ((sock_fd = socket(AF_NETLINK, SOCK_RAW, MYPROTO)) < 0)
    {
        perror("unetlink: socket");
        return sock_fd;
    }

    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_pid = getpid();     /* self pid */

    if (bind(sock_fd, (struct sockaddr *)&addr,
                sizeof addr) == -1)
    {
        close(sock_fd);
        perror("unetlink: bind");
        return -1;
    }

    if (setsockopt(sock_fd, 270, NETLINK_ADD_MEMBERSHIP,
                &group, sizeof(group)) < 0)                 /* Requires root privileges */
    {
        perror("setsockopt");
        return -1;
    }
    return sock_fd;
}


// TODO: Store the bus IDs in an array list of type char *, rather than printing them out

/*
* Iterates over input devices within /sys/clas/input and
* stores the bus IDs of each device with the name event*
*/
void acquire_bus_ids(
    struct udev *udev_ctx,
    struct udev_list_entry *devs,
    struct ID_List *list)
{
    struct udev_device *device;             /* udev device object */
    struct udev_list_entry *dlist_entry;    /* current device list entry */

    /* Iterate over the device list and store the respective bus IDs */
    udev_list_entry_foreach(dlist_entry, devs)
    {
        char curr_id[12];           /* Current bus ID */
        char curr_devpath[1024];    /* Current device path */

        const char *path;
        const char *dev_name;

        // Acquire a device handle through a syspath, e.g. /sys/input/event0
        path = udev_list_entry_get_name(dlist_entry);
        device = udev_device_new_from_syspath(udev_ctx, path);
        
        dev_name = udev_device_get_sysname(device);     /* The actual device name, e.g. event0 */

        /* Only worry about event* device names */
        if (strstr(dev_name, "event"))
        {
            debug("I: DEVNAME=%s", udev_device_get_sysname(device));

            memset(&curr_id, '\0', sizeof(curr_id));
            memset(&curr_devpath, '\0', sizeof(curr_devpath));

            // Get the current device path
            strncpy(curr_devpath, udev_device_get_devpath(device), sizeof(curr_devpath) - 1);
            curr_devpath[sizeof(curr_devpath) - 1] = '\0';

            find_bus_id(curr_id, curr_devpath);
            if (curr_id[0] == '\0')
            {
                debug("Failed to parse the bus id for %s", dev_name);
                continue;
            }

            // TODO: id_list_add call below segfaults upon first invocation
            //      find_bus_id() and id_list_add() are the current culprits

            debug("Found bus_id %s for %s\n", curr_id, dev_name);
            id_list_add(list, curr_id);
        }
        udev_device_unref(device);
    }
}

int main()
{
    int nls;
    int valid = 1;

    char packet[MAX_PAYLOAD];

    FILE *unbind_fp;

    int is_first_stamp = 1;
    int sequence_count = 0;
    unsigned long stroke_separation = 0;  /* Individual stroke separation measurement */
    long separation_sum = 0;              /* Running total of stroke separation measurements */
    double avg_separation = 0;            /* Average stroke separation used for keypress validation */

    if ((nls = open_netlink()) < 0)
        return nls;

    while (1)
    {
        /* Get the keypress packet */
        memset(&packet, '\0', sizeof packet);
        recv_packet(nls, packet);
        if (packet[0] == '\0')      /* Packet read failed */
        {
            log_err("[!] Failed to read packet");
        }

        debug();
        debug("Packet contents: %s", packet);
        debug("Sequence count: %d", sequence_count);

        if (is_first_stamp)    /* Do not add the very first stamp to the total */
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
            avg_separation = (double)separation_sum / (MAX_SEQ - 1); /* Calculate avg of 3 separation measurements */
            debug("Average key separation: %.2f", avg_separation);
            valid = validate_packet(&avg_separation);

            // Reset total, average, and sequence count
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

            /* Create udev context */
            udev = udev_new();
            if (!udev)
            {
                debug("Failed to create udev context.");
                return -1;
            }

            /* Create enumerate context */
            enumerate = udev_enumerate_new(udev);
            if (!enumerate)
            {
                debug("Failed to create enumerate context.");
                udev_unref(udev);
                return -1;
            }

            /* Specify subsystem as input and scan for devices */
            udev_enumerate_add_match_subsystem(enumerate, "input");
            udev_enumerate_scan_devices(enumerate);

            /* Populate the device list */
            devices = udev_enumerate_get_list_entry(enumerate);
            if (!devices)
            {
                debug("Failed to populate the device list");
                udev_enumerate_unref(enumerate);
                udev_unref(udev);
            }

            /* Iterate over the device list and store the respective bus IDs */
            struct ID_List *id_list = id_list_create();
            acquire_bus_ids(udev, devices, id_list);
            
            debug("Printing out the id_list ...");
            for (int i = 0; i < id_list_size(id_list); i++)
            {
                debug("ID: %s", id_list_get(id_list, i));
            }

            udev_enumerate_unref(enumerate);
            udev_unref(udev);

            // TODO: Unbind the drivers by writing to the appropriate /sys entry
            unbind_fp = fopen(UNBIND_PATH, "w");
            if (!unbind_fp)
            {
                log_err("Failed to open UNBIND_PATH for writing");
                continue;
            }
            
            for (int i = 0; i < id_list_size(id_list); i++)
            {
                char *curr_id = id_list_get(id_list, i);
                if (write_to_sys(unbind_fp, curr_id) < 0)
                {
                    debug("Failed to write %s to %s",
                        curr_id, UNBIND_PATH
                    );
                }
            }

            id_list_destroy(id_list);
        }
        valid = 1;
    }
    fclose(unbind_fp);
    close(nls);

    return 0;
}