#ifndef BIND_MGMT_H
#define BIND_MGMT_H

#include <libudev.h>
#include <regex.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "dbg.h"

#define SYS_PATH    "/sys/class/input"
#define UNBIND_PATH "/sys/bus/usb/drivers/usbhid/unbind"
#define BIND_PATH   "/sys/bus/usb/drivers/usbhid/bind"
#define MAX_MATCHES 1


// TODO: Implement the driver unbinding operation

/*
* Uses regex to find the bus id from the given device path
*/
void find_bus_id(char *result, char *dev_path)
{
    regex_t regex;
    int ret;

    regmatch_t matches[MAX_MATCHES];    // List of matches

    /* Compile the regular expression */
    ret = regcomp(&regex, "1-[0-9|.]+:[0-9|.]+", REG_EXTENDED);
    if (ret != 0)
    {
        debug("Failed to compile regex");
        goto cleanup;
    }

    ret = regexec(&regex, dev_path, MAX_MATCHES, matches, 0);
    if (ret == 0 && matches[0].rm_eo != 0)                      /* Successful exectuion and match was found */
    {
        debug("\"%s\" matches characters %d - %d", dev_path, matches[0].rm_so, matches[0].rm_eo);

        // Form the substring based on the match offsets
        int substr_length = matches[0].rm_eo - matches[0].rm_so;    
        strncpy(result, dev_path + matches[0].rm_so, substr_length);
        result[strlen(result)] = '\0';
    }

    cleanup:
    regfree(&regex);
}

/*
* Writes the bus id to the UNBIND_PATH,
* effectively unbinding the HID driver from the respective device
*/
int write_to_sys(FILE *file, char* bus_id)
{
    size_t num_written;

    // NOTE: You may need to append a '\n' to the bus_id string
    num_written = fprintf(file, "%s", bus_id);
    if (num_written < strlen(bus_id))
    {
        return -1;
    }
    return 0;
}


/*
* Handles the unbinding of device drivers from input devices
*/
// int unbind_handler()
// {
//     FILE *fp;

//     struct udev *udev;
//     struct udev_device *dev;
//     char device[128];
//     char dev_path[1024];
//     char bus_id[12];

//     int status = -1;            /* Denotes error or success */

//     /* Initialize the bus_id variable for later use */
//     memset(&bus_id, '\0', sizeof(bus_id));

//     /* Build device path using SYS_PATH */
//     snprintf(device, sizeof(device), "%s/%s", SYS_PATH, "event0");

//     // Store the dev_path attribute for later use
//     strncpy(dev_path, udev_device_get_devpath(dev), sizeof(dev_path) - 1);
//     dev_path[sizeof(dev_path) - 1] = '\0';          /* Append the null-byte for the last byte */

//     /* Retrieve the bus_id found in dev_path */
//     find_bus_id(bus_id, dev_path);
//     if (bus_id[0] == '\0')         /* Error occurred in finding the bus_id */
//     {
//         debug("Failed to parse the bus id");
//         goto cleanup;
//     }

//     // Open the UNBIND_PATH for writing
//     fp = fopen(UNBIND_PATH, "w");
//     if (!fp)
//     {
//         debug("Failed to open UNBIND_PATH for writing");
//         goto cleanup;
//     }

//     // Attempt to unbind the driver from the device
//     status = write_to_sys(fp, bus_id);
//     if (status < 0)
//     {
//         debug("Failed to write to UNBIND_PATH");
//         goto cleanup;
//     }
//     status = 0;

//     cleanup:
//     udev_device_unref(dev);     /* free dev */
//     udev_unref(udev);           /* free udev */
//     fclose(fp);                 /* close file stream */
    
//     return status;
// }
#endif  // BIND_MGMT_H