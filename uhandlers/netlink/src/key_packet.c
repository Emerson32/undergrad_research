#include <linux/netlink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MYPROTO     NETLINK_USERSOCK
#define MYGRP       31

#define MAX_PAYLOAD         1024
#define SEPARATION_THRESH   80

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


int recv_packet(int sock, char *packt_buff)
{
    struct sockaddr_nl nladdr;
    struct msghdr msg;
    struct iovec iov;
    int ret;
    char buff[MAX_PAYLOAD];

    memset(&buff, '\0', sizeof buff);

    iov.iov_base = (void *)buff;
    iov.iov_len = sizeof(buff);
    msg.msg_name = (void *)&nladdr;
    msg.msg_namelen = sizeof(nladdr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    if ((ret = recvmsg(sock, &msg, 0)) < 0)
        return -1;
 
    // Copy the data into the provided packet buffer
    strncpy(
        packt_buff,
        (char *)NLMSG_DATA((struct nlmsghdr *) &buff), MAX_PAYLOAD);
    
    return 0;
}

unsigned long get_separation(char *packet)
{
    char *token;
    char *packet_copy;
    char *rest;
    signed long stroke_separation = 0;

    int token_count = 0;

    packet_copy = strdup(packet);
    rest = packet_copy;

    while ((token = strtok_r(rest, " ", &rest)))
    {
        token_count++;

        if (token_count == 2)    /* Stroke separation found */
            break;
    }

    /* Parsing error occurred */
    if (!token)
        return -1;

    sscanf(token, "%ld", &stroke_separation);
    free(packet_copy);

    return stroke_separation;
}

int validate_packet(double *avg_sep)
{
    if (*avg_sep < SEPARATION_THRESH)
        return 0;

    return 1;
}