#include <libudev.h>
#include <regex.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "dbg.h"

#define UNBIND_PATH "/sys/bus/usb/drivers/usbhid/unbind"
#define BIND_PATH   "/sys/bus/usb/drivers/usbhid/bind"
#define MAX_MATCHES 1

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