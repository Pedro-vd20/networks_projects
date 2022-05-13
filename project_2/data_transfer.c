#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

#include "constants.h"
#include "data_transfer.h"

int receive_file(int sockfd, char *filename)
{
    FILE *fp;
    fp = fopen(filename, "wb");
    if (fp == NULL)
    {
        perror("file nout found");
        return 0;
    }
    size_t n; // Does this have to int or size_t?
    char buffer[1024];
    int offset = 0; // Does this have to int or size_t?
    char response[256];
    int code;

    while (1)
    {
        int32_t temp_size;
        int size;
        bzero(&temp_size, sizeof(temp_size));
        recv(sockfd, &temp_size, sizeof(temp_size), 0);
        size = ntohl(temp_size);

        bzero(buffer, sizeof(buffer));
        n = recv(sockfd, buffer, size, 0);

        if (n <= 0)
        {
            fclose(fp);
            break;
        }
        fseek(fp, offset, SEEK_SET);
        if (strcmp(buffer, TRANSFER_COMPLETE) == 0)
        {
            printf("%s", TRANSFER_COMPLETE);
        }
        else
        {
            fwrite(buffer, 1, n, fp);
        }
        // is n equal to 1024 in most cases?
        offset += n;
    }
    fclose(fp);
    return 0;
}

int send_file(int sockfd, char *filename)
{

    FILE *fp;
    fp = fopen(filename, "r");
    if (fp == NULL)
    {
        perror("file not found");
        return 0;
    }
    int n;
    char buffer[1024];
    int bytes_read;
    int bytes_sent;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0)
    {

        int i = 0;
        do
        {
            int32_t convert_size = htonl(bytes_read);
            send(sockfd, &convert_size, sizeof(convert_size), 0);
            bytes_sent += send(sockfd, buffer, bytes_read, 0);
            bytes_read = bytes_read - bytes_sent;

        } while (bytes_read > 0);
        bzero(buffer, sizeof(buffer));
    }

    char success_response[256] = TRANSFER_COMPLETE;
    printf("%s", TRANSFER_COMPLETE);
    int bytes_response = 23;
    int32_t response_size = htonl(bytes_response);
    send(sockfd, &response_size, sizeof(response_size), 0);
    send(sockfd, success_response, sizeof(success_response), 0);
    fclose(fp);

    return 0;
}
