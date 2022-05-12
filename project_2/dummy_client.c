#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#define SIZE 1024

int send_file(int sockfd, char *filename)
{

    printf("starting send file \n");
    FILE *fp;
    fp = fopen(filename, "r");
    if (fp == NULL)
    {
        perror("file nout found");
        return 0;
    }
    int n;
    char buffer[1024];
    int bytes_read;
    int bytes_sent;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0)
    {
        printf("bytes_read %d \n", bytes_read);
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

    char success_response[256] = "226 Transfer completed.";
    int bytes_response = 23;
    int32_t response_size = htonl(bytes_response);
    send(sockfd, &response_size, sizeof(response_size), 0);
    send(sockfd, success_response, sizeof(success_response), 0);
    close(fp);
    close(sockfd);

    return 0;
}

int main()
{
    char *ip = "127.0.0.1";
    int port = 8080;
    int e;

    int sockfd;
    struct sockaddr_in server_addr;
    FILE *fp;
    char *filename = "dummy_send.txt";

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

    e = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (e == -1)
    {
        perror("[-]Error in socket");
        exit(1);
    }
    printf("[+]Connected to Server.\n");

    // fp = fopen(filename, "r");
    // if (fp == NULL)
    // {
    //     perror("[-]Error in reading file.");
    //     exit(1);
    // }

    send_file(sockfd, filename);
    printf("[+]File data sent successfully.\n");

    printf("[+]Closing the connection.\n");

    return 0;
}