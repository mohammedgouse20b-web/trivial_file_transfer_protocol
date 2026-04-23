#include "tftp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>

/* Function prototype */
void handle_client(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, tftp_packet *packet);

int main()
{
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    tftp_packet packet;

    /* Create UDP socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    /* Set timeout */
    struct timeval timeout = {TIMEOUT_SEC, 0};
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    /* Setup server address */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    /* Bind socket */
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)

    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    printf("TFTP Server listening on port %d...\n", PORT);

    /* Infinite loop */
    while (1)
    {
        int n = recvfrom(sockfd, &packet, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &client_len);

        if (n < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)

                continue;
            perror("recvfrom failed");
            continue;
        }

        handle_client(sockfd, client_addr, client_len, &packet);
    }
}

/* Handle client request */
void handle_client(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, tftp_packet *packet)

{
    unsigned short opcode = ntohs(packet->opcode);

    char *filename = packet->body.request.filename;

    if (opcode == RRQ)
    {
        printf("Sending file: %s\n", filename);
        send_file(sockfd, client_addr, client_len, filename, 1);

        printf("File sent successfully.\n\n");
    }
    else if (opcode == WRQ)
    {
        printf("Receiving file: %s\n", filename);

        /* Send ACK(0) */
        tftp_packet ack;
        memset(&ack, 0, sizeof(ack));
        ack.opcode = htons(ACK);
        ack.body.ack_packet.block_number = htons(0);

        sendto(sockfd, &ack, 4, 0, (struct sockaddr *)&client_addr, client_len);

        receive_file(sockfd, client_addr, client_len, filename);

        printf("File received successfully.\n\n");
    }
    else
    {
        send_error(sockfd, client_addr, client_len, 4, "Illegal TFTP operation");
    }
}
