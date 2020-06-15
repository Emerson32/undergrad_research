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
#define MAX_SEQ      4

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
    int valid = 1;

    char packet[MAX_PAYLOAD];

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
        if (packet[0] == '\0')      /* packet read failed */
        {
            log_err("[!] Failed to read packet");
        }

        printf("\n");
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
            printf("[+] Invalid key sequence detected\n");

            if (binding_handler() < 0)
                return -1;
        }

        valid = 1;
    }
    close(nls);

    return 0;
}