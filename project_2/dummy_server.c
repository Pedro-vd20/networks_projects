#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#define SIZE 1024

int parse_response(char *msg)
{
    // collect first word
    char *token = strtok(msg, " ");
    if (token == NULL)
    {
        return -1;
    }

    // check if token is a number
    for (int i = 0; i < strlen(token); ++i)
    {
        if (token[i] < '0' || token[i] > '9')
        {
            return -1;
        }
    }

    return atoi(token);
}

int receive_file(int sockfd, char *filename)
{
    printf("starting receive file .... \n"); //**********DELETE
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
        printf("sizeOfBuffer %d \n", size);
        bzero(buffer, sizeof(buffer));
        n = recv(sockfd, buffer, size, 0);
        printf("buffer: %s \n", buffer);
        if (n <= 0)
        {
            fclose(fp);
            printf("done reading file \n");
            break;
        }
        fseek(fp, offset, SEEK_SET);
        fwrite(buffer, 1, n, fp); // is n equal to 1024 in most cases?
        offset += n;
        
    }

    return 0;
}

int main()
{
    char *ip = "127.0.0.1";
    int port = 8080;
    int e;

    int sockfd, new_sock;
    struct sockaddr_in server_addr, new_addr;
    socklen_t addr_size;
    char buffer[SIZE];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("[-]Error in socket");
        exit(1);
    }
    printf("[+]Server socket created successfully.\n");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = port;
    server_addr.sin_addr.s_addr = inet_addr(ip);

    e = bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (e < 0)
    {
        perror("[-]Error in bind");
        exit(1);
    }
    printf("[+]Binding successfull.\n");

    if (listen(sockfd, 10) == 0)
    {
        printf("[+]Listening....\n");
    }
    else
    {
        perror("[-]Error in listening");
        exit(1);
    }

    addr_size = sizeof(new_addr);
    new_sock = accept(sockfd, (struct sockaddr *)&new_addr, &addr_size);
    receive_file(new_sock, "thebiblereceived.txt");
    printf("[+]Data written in the file successfully.\n");

    return 0;
}