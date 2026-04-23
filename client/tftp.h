#ifndef TFTP_H
#define TFTP_H

#include <stdint.h>
#include <arpa/inet.h>

/* Default server port */
#define PORT 7008

/* Max TFTP packet size:
   2 bytes opcode + 2 bytes block + 512 bytes data */
#define BUFFER_SIZE 516

/* Socket timeout in seconds */
#define TIMEOUT_SEC 5

/* TFTP Operation Codes */
typedef enum
{
    RRQ = 1,  // Read Request (GET)
    WRQ = 2,  // Write Request (PUT)
    DATA = 3, // Data Packet
    ACK = 4,  // Acknowledgment
    ERROR = 5 // Error Packet
} tftp_opcode;

/* TFTP Packet Structure */
typedef struct
{
    unsigned short opcode;

    union
    {
        /* Used for RRQ / WRQ */
        struct
        {
            char filename[256];
            char mode[8]; // "octet"
        } request;

        /* Used for DATA */
        struct
        {
            unsigned short block_number;
            char data[512];
        } data_packet;

        /* Used for ACK */
        struct
        {
            unsigned short block_number;
        } ack_packet;

        /* Used for ERROR */
        struct
        {
            unsigned short error_code;
            char error_msg[512];
        } error_packet;

    } body;

} tftp_packet;

/* Function declarations */
void send_file(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, char *filename, int mode);

void receive_file(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, char *filename);

void send_error(int sockfd, struct sockaddr_in addr, socklen_t addr_len, unsigned short error_code, const char *msg);

#endif
