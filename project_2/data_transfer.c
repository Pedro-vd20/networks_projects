#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

#include "constants.h"
#include "data_transfer.h"

int receive_file(int sockfd, char *filename)
{
    printf("Receiving %s\n", filename);

    // open file
    FILE* fp = fopen(filename, "wb");
    
    /*printf("starting receive file .... \n"); //**********DELETE
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
        int size = 0;
        recv(sockfd, &size, sizeof(size), 0);
        
        bzero(buffer, sizeof(buffer));
        n = recv(sockfd, buffer, size, 0);
        if (strcmp(buffer, TRANSFER_COMPLETE) == 0)
            printf("%s \n", buffer);

        if (n <= 0)
        {
            printf("done reading file \n");
            break;
        }
        // fseek(fp, offset, SEEK_SET);
        fwrite(buffer, 1, n, fp); // is n equal to 1024 in most cases?
        // offset += n;
    }
    fclose(fp);
    return 0;*/
}

int send_file(int sockfd, char *filename)
{
    printf("Sending %s\n", filename);

    FILE* fp = fopen(filename, "rb");
    if(fp == NULL)
    {
        perror("file not found");
        return 0;
    }

    int bytes_read;
    int bytes_sent;

    char buffer[1024];
    while(1) {
        // clear buffer
        bzero(buffer, sizeof(buffer));

        // read file
        bytes_read = fread(buffer, sizeof(buffer), 1, fp);
        if(bytes_read == 0) {
            break;
        }

        // send info
        bytes_sent = 0;
        do {
            bytes_sent += send(sockfd, buffer + bytes_sent, bytes_read - bytes_sent, 0);
        } while(bytes_sent < bytes_read);
    }

    // Send file transmission finished signal
    send(sockfd, TRANSFER_COMPLETE, LEN_TRANSFER_COMPLETE, 0);
    fclose(fp);

    return 0;
}