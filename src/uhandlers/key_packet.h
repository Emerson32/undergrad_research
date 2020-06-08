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

#define MAX_PAYLOAD  1024


void read_packet(int sock, char *packt_buff)
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
    {
        perror("recvmsg");
        return;
    } 
 
    // Copy the data into the provided packet buffer
    strncpy(packt_buff, (char *)NLMSG_DATA((struct nlmsghdr *) &buff),
            sizeof(packt_buff) - 1);
}

int validate_packet(char *packet)
{
    // Packet format: key_code validity_flag
    char *token;
    char delim[2] = " ";

    /* Keypress */
    token = strtok(packet, delim);

    /* Validity flag */
    token = strtok(NULL, delim);

    /* Validity flag not found in this packet */
    if (!token)
    {
        return -1;
    }
    
    if (strcmp(token, "1") == 0)
        return 1;

    return 0;
}


#endif // KEY_PACKET_H