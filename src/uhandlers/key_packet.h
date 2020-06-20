/*
* Procedures for receiving and validating a kepyress packet
*/

#ifndef KEY_PACKET_H
#define KEY_PACKET_H

#include <linux/netlink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_PAYLOAD         1024
#define SEPARATION_THRESH   80

/*
* Responsible for receiving a keypress packet from kernel space
*/
void recv_packet(int sock, char *packt_buff)
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

    //printf("Listening for keystroke packets...\n");
    if ((ret = recvmsg(sock, &msg, 0)) < 0)
        return;
 
    // Copy the data into the provided packet buffer
    strncpy(packt_buff, (char *)NLMSG_DATA((struct nlmsghdr *) &buff),
            MAX_PAYLOAD);
}

/* 
* Responsible for retrieving the stroke separation measurement from each packet
*/
unsigned long get_separation(char *packet)
{
    char *token;
    char delim[2] = " ";
    signed long stroke_separation = 0;

    /* Keypress value */
    token = strtok(packet, delim);

    /* Stroke separation from last keypress */
    token = strtok(NULL, delim);

    /* Parsing error occurred */
    if (!token)
    {
        return -1;
    }

    sscanf(token, "%ld", &stroke_separation);

    return stroke_separation;
}

/*
* Responsible for the validation of a keypress packet
*/
int validate_packet(double *avg_sep)
{
    if (*avg_sep < SEPARATION_THRESH)
        return 0;

    return 1;
}

#endif // KEY_PACKET_H