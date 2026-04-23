#include "tftp.h"
#include "tftp_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <dirent.h>

/* Function to display main menu */
void show_menu()
{
    printf("\n====================================\n");
    printf("              TFTP MENU\n");
    printf("====================================\n");
    printf("1. Connect to Server\n");
    printf("2. Upload File (PUT)\n");
    printf("3. Download File (GET)\n");
    printf("4. Change Mode\n");
    printf("5. Exit\n");
    printf("====================================\n");
    printf("\nEnter your choice: ");
}

/* Function to display available local files */
void list_local_files()
{
    DIR *d = opendir(".");
    struct dirent *dir;

    if (d)
    {
        printf("\nAvailable files in client directory:\n");
        while ((dir = readdir(d)) != NULL)
        {
            /* DT_REG ensures we print only regular files */
            if (dir->d_type == DT_REG)
                printf(" - %s\n", dir->d_name);
        }
        closedir(d);
    }
}

/* Connect to server */
void connect_to_server(tftp_client_t *client, char *ip, int port)
{
    /* Create UDP socket */
    client->sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (client->sockfd < 0)
    {
        perror("Socket creation failed");
        return;
    }

    /* Set timeout */
    struct timeval timeout = {TIMEOUT_SEC, 0};

    /* ✅ FIX: pass &timeout and its size */
    setsockopt(client->sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    /* Configure server address */
    memset(&client->server_addr, 0, sizeof(client->server_addr));

    client->server_addr.sin_family = AF_INET;
    client->server_addr.sin_port = htons(port);

    /* Convert IP string to binary */
    if (inet_pton(AF_INET, ip, &client->server_addr.sin_addr) <= 0)

    {
        printf("Invalid IP address\n");
        return;
    }

    client->server_len = sizeof(client->server_addr);

    printf("Connected successfully to %s:%d\n", ip, port);
}

/* PUT operation */
void put_file(tftp_client_t *client, char *filename)

{
    /* Send WRQ request */
    tftp_packet packet;
    memset(&packet, 0, sizeof(packet));

    packet.opcode = htons(WRQ);
    strcpy(packet.body.request.filename, filename);

    strcpy(packet.body.request.mode, "octet");

    int len = 2 + strlen(filename) + 1 + strlen("octet") + 1;

    sendto(client->sockfd, &packet, len, 0, (struct sockaddr *)&client->server_addr, client->server_len);

    /* Wait for ACK(0) */
    tftp_packet ack;
    int n = recvfrom(client->sockfd, &ack, sizeof(ack), 0, NULL, NULL);

    if (n < 0 || ntohs(ack.opcode) != ACK || ntohs(ack.body.ack_packet.block_number) != 0)
    {
        printf("WRQ failed.\n");
        return;
    }

    printf("Uploading file...\n");

    /* Send actual file data */
    send_file(client->sockfd, client->server_addr, client->server_len, filename, client->mode);

    printf("Upload completed.\n");
}

/* GET operation */
void get_file(tftp_client_t *client, char *filename)

{
    /* Send RRQ request */
    tftp_packet packet;
    memset(&packet, 0, sizeof(packet));

    packet.opcode = htons(RRQ);
    strcpy(packet.body.request.filename, filename);

    strcpy(packet.body.request.mode, "octet");

    int len = 2 + strlen(filename) + 1 + strlen("octet") + 1;

    sendto(client->sockfd, &packet, len, 0, (struct sockaddr *)&client->server_addr, client->server_len);

    printf("Downloading file...\n");

    /* Receive file from server */
    receive_file(client->sockfd, client->server_addr, client->server_len, filename);

    printf("Download completed.\n");
}

/* Main function */
int main()
{
    tftp_client_t client;
    memset(&client, 0, sizeof(client));

    /* Default mode = Normal */
    client.mode = 1;

    int choice;
    char filename[256];
    char ip[INET_ADDRSTRLEN];

    while (1)
    {
        show_menu();
        scanf("%d", &choice);

        switch (choice)
        {
        case 1:
            printf("Enter Server IP: ");
            scanf("%s", ip);
            connect_to_server(&client, ip, PORT);

            break;

        case 2:
            list_local_files();
            printf("Enter file to upload: ");
            scanf("%s", filename);

            /* Validate file existence */
            if (access(filename, F_OK) != 0)
            {
                printf("File does not exist in client.\n");
                break;
            }

            put_file(&client, filename);
            break;

        case 3:
            printf("Enter file to download: ");
            scanf("%s", filename);
            get_file(&client, filename);
            break;

        case 4:
            printf("\nSelect Mode:\n");
            printf("1. Normal (512 bytes)\n");
            printf("2. Octet (1 byte)\n");
            printf("3. Netascii\n");
            printf("Enter choice: ");
            scanf("%d", &client.mode);
            printf("Mode updated successfully.\n");
            break;

        case 5:
            printf("Exiting client...\n");
            exit(0);

        default:
            printf("Invalid option.\n");
        }
    }

    return 0;
}
