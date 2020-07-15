/*
* Procedures for receiving and validating a kepyress packet
*/
#ifndef KEY_PACKET_H
#define KEY_PACKET_H

/*
* Responsible for establishing a netlink listener for keystrokes
* sent from kernel space
*/
int open_netlink(void);

/*
* Responsible for receiving a keypress packet sent from kernel space
*/
int recv_packet(int sock, char *packt_buff);

/* 
* Responsible for retrieving the stroke separation measurement from each packet
*/
unsigned long get_separation(char *packet);

/*
* Responsible for the validation of a keypress packet
*/
int validate_packet(double *avg_sep);

#endif // KEY_PACKET_H