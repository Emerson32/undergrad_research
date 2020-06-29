#include <regex.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include "dbg.h"

#define MAX_MATCHES 1
#define MAX_IDBUFF  12

int find_bus_id(char *result, char *dev_path)
{
    regex_t regex;
    int ret;

    regmatch_t matches[MAX_MATCHES];    // List of matches

    /* Compile the regular expression */
    ret = regcomp(&regex, "1-[0-9|.]+:[0-9|.]+", REG_EXTENDED);
    if (ret != 0)
    {
        debug("Failed to compile regex");
        return -1;
    }

    ret = regexec(&regex, dev_path, MAX_MATCHES, matches, 0);

    /* Successful exectuion and match was found */
    if (ret == 0 && matches[0].rm_eo != 0) 
    {
        debug("\"%s\" matches characters %d - %d",
                dev_path, matches[0].rm_so, matches[0].rm_eo);

        // Form the substring based on the match offsets
        int substr_length = matches[0].rm_eo - matches[0].rm_so;    
        strncpy(result, dev_path + matches[0].rm_so, substr_length);
        result[strlen(result)] = '\0';
    }

    regfree(&regex);
    return 0;
}

int write_to_sys(FILE **file, char **bus_id)
{
    size_t num_written;

    if (setvbuf(*file, *bus_id, _IONBF, MAX_IDBUFF) !=0)
    {
        return -1;
    }

    num_written = fprintf(*file, "%s", *bus_id);
    if (num_written < strlen(*bus_id))
    {
        return -1;
    }

    return 0;
}