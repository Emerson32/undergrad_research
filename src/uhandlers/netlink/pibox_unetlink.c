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

void read_keypress(int sock)
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
        perror("recvmsg");
    else
        printf("%s\n", (char *)NLMSG_DATA((struct nlmsghdr *) &buff));
}

int main()
{
    int nls;
    if ((nls = open_netlink()) < 0)
        return nls;

    while (1)
    {
        read_keypress(nls);
    }
    return 0;
}
