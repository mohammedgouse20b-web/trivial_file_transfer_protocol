#ifndef TFTP_CLIENT_H
#define TFTP_CLIENT_H

#include <arpa/inet.h>

typedef struct {
    int sockfd;
    struct sockaddr_in server_addr;
    socklen_t server_len;
    int mode; // 1=Normal, 2=Octet, 3=Netascii
} tftp_client_t;

void connect_to_server(tftp_client_t *client, char *ip, int port);
void put_file(tftp_client_t *client, char *filename);
void get_file(tftp_client_t *client, char *filename);

#endif
