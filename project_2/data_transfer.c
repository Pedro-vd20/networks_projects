#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

#include "data_transfer.h"

int receive_file(int sockfd, char *filename)
{
    printf("starting receive file .... \n"); //**********DELETE
    FILE *fp;
    fp = fopen(filename, "w");
    if (fp == NULL)
    {
        perror("file nout found");
        return -1;
    }
    size_t n; // Does this have to int or size_t?
    char buffer[1024];
    int offset = 0; // Does this have to int or size_t?
    while (1)
    {
        n = recv(sockfd, buffer, 1024, 0);
        printf("n: %zu \n", n);
        if (n <= 0)
        {
            fclose(fp);
            printf("done reading file \n");
            break;
        }
        fseek(fp, offset, SEEK_SET);
        fwrite(buffer, 1, n, fp); // is n equal to 1024 in most cases?
        offset += n;
        bzero(buffer, 1024);
    }
    printf("ending receive file .... \n"); //**********DELETE
    return 0;
}

int send_file(int sockfd, char *filename)
{

    printf("starting send file \n");
    FILE *fp;
    fp = fopen(filename, "r");
    if (fp == NULL)
    {
        perror("file nout found");
        return -1;
    }
    int n;
    char buffer[1024];
    size_t bytes_read = 0;
    // while ()
    // {
    // }
    printf("ending send file");
    return 1;
}