/*
 * User-space netlink listener
 */

#include <linux/netlink.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "dbg.h"
#include "key_packet.h"

#define MYPROTO      NETLINK_USERSOCK
#define MYGRP        31
#define MAX_PAYLOAD  1024

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
                &group, sizeof(group)) < 0)
    {
        perror("setsockopt");
        return -1;
    }
    return sock_fd;
}

int main()
{
    int nls;
    int valid;
    char packet[MAX_PAYLOAD];

    if ((nls = open_netlink()) < 0)
        return nls;

    while (1)
    {
        /* Get the keypress packet */
        memset(&packet, '\0', sizeof packet);
        read_packet(nls, packet);
        if (packet[0] == '\0')      /* packet read failed */
        {
            log_err("[!] Failed to read packet");
        }

        debug("Packet contents: %s", packet);

        if ((valid = validate_packet(packet)) < 0)
        {
            log_err("[!] Validity flag missing");
        }

        if (valid)
        {
            debug("Valid packet found: %s", packet);
        }
        
        /* Temporarily unbind input event drivers */
        else       
        {
            // TODO: Implement driver unbinding feature
            printf("[+] Invalid key detected\n");  
        }
    }
    return 0;
}