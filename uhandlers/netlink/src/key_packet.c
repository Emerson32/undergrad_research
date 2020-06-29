#include <linux/netlink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_PAYLOAD         1024
#define SEPARATION_THRESH   80

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