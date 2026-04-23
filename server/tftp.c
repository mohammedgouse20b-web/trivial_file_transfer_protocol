/* Common file for server & client */

#include <stdio.h>
#include <string.h>
#include "tftp.h"

/* ===================== SEND FILE ===================== */
void send_file(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, char *filename, int mode)

{
    FILE *fp = fopen(filename, "rb");
    if (!fp)
    {
        send_error(sockfd, client_addr, client_len, 1, "File not found");

        return;
    }

    tftp_packet packet;
    unsigned short block = 1;
        
    size_t bytes_read;

    size_t chunk = (mode == 2) ? 1 : 512;

    while (1)
    {
        memset(&packet, 0, sizeof(packet));

        packet.opcode = htons(DATA);
        packet.body.data_packet.block_number = htons(block);

        bytes_read = fread(packet.body.data_packet.data, 1, chunk, fp);

        sendto(sockfd, &packet, 4 + bytes_read, 0, (struct sockaddr *)&client_addr, client_len);

        tftp_packet ack;
        int n = recvfrom(sockfd, &ack, sizeof(ack), 0, NULL, NULL);

        if (n < 0)
        {
            fseek(fp, -bytes_read, SEEK_CUR);
            continue;
        }

        if (ntohs(ack.opcode) != ACK || ntohs(ack.body.ack_packet.block_number) != block)

        {
            fseek(fp, -bytes_read, SEEK_CUR);
            continue;
        }

        /* Print dot per byte in octet mode */
        if (mode == 2 && bytes_read == 1)
        {
            printf(".");
            fflush(stdout);
        }

        if (bytes_read == 0)
            break;

        block++;
    }

    if (mode == 2)
        printf("\n");

    fclose(fp);
}

/* ===================== RECEIVE FILE ===================== */
void receive_file(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, char *filename)

{
    FILE *fp = fopen(filename, "wb");
    if (!fp)
    {
        perror("File create failed");
        return;
    }

    tftp_packet packet;
    unsigned short expected_block = 1;

    while (1)
    {
        int n = recvfrom(sockfd, &packet, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &client_len);

        if (n < 0)
            continue;

        if (ntohs(packet.opcode) == ERROR)
        {
            printf("TFTP ERROR: %s\n", packet.body.error_packet.error_msg);

            fclose(fp);
            return;
        }

        if (ntohs(packet.opcode) != DATA)
            continue;

        if (ntohs(packet.body.data_packet.block_number) != expected_block)

            continue;

        size_t data_len = n - 4;

        fwrite(packet.body.data_packet.data, 1, data_len, fp);

        tftp_packet ack;
        memset(&ack, 0, sizeof(ack));
        ack.opcode = htons(ACK);
        ack.body.ack_packet.block_number = htons(expected_block);

        sendto(sockfd, &ack, 4, 0, (struct sockaddr *)&client_addr, client_len);

        if (data_len == 0)
            break;

        expected_block++;
    }

    fclose(fp);
}

/* ===================== SEND ERROR ===================== */
void send_error(int sockfd, struct sockaddr_in addr, socklen_t addr_len, unsigned short error_code, const char *msg)

{
    tftp_packet err;
    memset(&err, 0, sizeof(err));

    err.opcode = htons(ERROR);
    err.body.error_packet.error_code = htons(error_code);

    strncpy(err.body.error_packet.error_msg, msg, sizeof(err.body.error_packet.error_msg) - 1);

    sendto(sockfd, &err, 4 + strlen(err.body.error_packet.error_msg) + 1, 0, (struct sockaddr *)&addr, addr_len);
}
