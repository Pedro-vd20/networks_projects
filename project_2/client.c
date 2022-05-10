
/*
    We can have a single extra file commands.c to do all command processing
        Check if command has right num of arguments
        Check if command is valid

        Possibly could return num
            -1 means error
            0 - n are for each ith command

        Manage response codes
*/

/*
Testing:
    1. Correct authentication
    2. Get file
        2.1 Correct port 20 stuff
        2.2 Correct multi-multi threading
        2.3 Correct merging of threads
        2.4 Correct sending of file
    3. Store file
        (should work similar to get file)
    4. List files
        4.1 Client
        4.2 Server
    5. CWD && PWD
        5.1 Change client directory
        5.2 Change server directory
    6. Quit



*/

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <stdlib.h>
#include "commands.h"
#include "threading.h"
#include "constants.h"
#include "user.h"

#define USERNAME_OK "331 Username OK, need password\n"
#define AUTHENTICATED "230 User logged in, proceed\n"
#define NOT_LOGGED_IN "530 Not logged in\n"
#define ERROR "501 Syntax error in parameters or arguments\n"
// -1  command not identified
//  *      -2 syntax error
//  *       0  PORT
// #define PORT 0
// #define USER 1
// #define PASS 2
// #define STOR 3
// #define RETR 4
// #define LIST 5
#define iLIST 6
// #define CWD 7
#define iCWD 8
// #define PWD 9
#define iPWD 10
// #define QUIT 11

/**
 * @brief maintains control of connection for each client
 *
 * @param arg client information
 */
void *handle_user(void *arg);

int main(int argc, char **argv)
{

    // if (argc != 3)
    // {
    //     fprintf(stderr, "usage: %s <IP Address> <Port> \n", argv[0]);
    //     exit(1);
    // }

    // Declare and verify socket file descriptor
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("Socket Error!");
        exit(1);
    }

    // Control connection to the server to port 21 and local host;
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    inet_aton("127.0.0.1", &server_address.sin_addr);
    server_address.sin_port = htons(2021);

    if (connect(sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        perror("connection error");
        return -1;
    }

    // get the port of the client after connect
    struct sockaddr_in client;
    socklen_t clientsz = sizeof(client);
    getsockname(sockfd, (struct sockaddr *)&client, &clientsz);

    printf("[%s:%u] > \n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));

    unsigned short control_port = ntohs(client.sin_port);
    // User Interface
    printf("control socket connected!");
    // menu
    printf("\n");
    printf("Hello!! Please Authenticate first  \n");
    printf("1. type \"USER\" followed by a space and your username \n");
    printf("2. type \"PASS\" followed by a space and your password \n");
    printf("\n");
    printf("Once Authenticated \n");
    printf("this is the list of commands : \n");
    printf("\"STOR\" to send a file to the server \n");
    printf("\"RETR\" to download a file from the server \n");
    printf("\"LIST\" to  to list all the files under the current server directory \n");
    printf("\"CWD\"  to change the current server directory \n");
    printf("\"PWD\" to display the current server directory \n");

    printf("Add \"!\" before the last three commands to apply them locally \n");

    int is_authenticated = 0;
    int is_username_ok = 0;
    int port_counter = 1;
    pthread_t thread_ids[NUM_THREADS];
    int busy[NUM_THREADS]; // keep track of threads currently in use
    bzero(busy, sizeof(busy));

    while (1)
    {
        char input[50] = "\0";
        fgets(input, 50, stdin);
        input[strcspn(input, "\n")] = 0;

        char copy_input[50];
        char data[40];
        strcpy(copy_input, input);

        int command_code = parse_command(copy_input, data);

        if (command_code < 0)
        {
            printf("%s", ERROR);
        }

        char response[100];

        if (command_code == USER && strlen(data) > 0)
        {

            send(sockfd, input, sizeof(input), 0);
            if (recv(sockfd, response, sizeof(response), 0) < 0)
            {
                perror("recv error!!");
                break;
            }

            if (parse_response(response) == 331)
            {
                is_username_ok = 1;
                printf("%s", USERNAME_OK);
            }
            else
            {
                printf("Username not registered! \n");
            }
        }
        else if (command_code == PASS && strlen(data) > 0 && is_username_ok)
        {
            send(sockfd, input, sizeof(input), 0);
            if (recv(sockfd, response, sizeof(response), 0) < 0)
            {
                perror("recv error!!");
                break;
            }

            if (parse_response(response) == 230)
            {
                printf("%s", AUTHENTICATED);
                is_authenticated = 1;
            }
            else if (parse_response(response) == 530)
            {
                printf("%s", NOT_LOGGED_IN);
            }
        }

        else if (is_authenticated)
        {

            if (command_code == STOR || command_code == RETR || command_code == LIST)
            {
                // openThread

                int t_id_index = open_thread(busy, NUM_THREADS);
                // int cntr_socket;
                // struct sockaddr_in address;
                // int counter;
                // unsigned short port;
                // char *input;

                thread_parameters data_transfer_info;
                data_transfer_info.cntr_socket = sockfd;
                data_transfer_info.address = server_address;
                data_transfer_info.port = control_port;
                data_transfer_info.input = input;
                data_transfer_info.counter = port_counter + control_port;
                data_transfer_info.command_code = command_code;
                // check if index -1 (figure out how to handle later)
                if (pthread_create(thread_ids + t_id_index, NULL, handle_user, &data_transfer_info) < 0)
                {
                    perror("multithreading");
                    return -1;
                }

                port_counter++;

                // close threads that finish running
                join_thread(thread_ids, busy, NUM_THREADS);
            }

            else if (command_code == PWD || command_code == CWD)
            {
                send(sockfd, input, sizeof(input), 0);
                char bufferResponse[1500];
                if (recv(sockfd, bufferResponse, sizeof(bufferResponse), 0) < 0)
                {
                    perror("recv issue, disconnecting");
                    break;
                }
                printf("%s\n", bufferResponse);
                bzero(&bufferResponse, sizeof(bufferResponse));
            }
            else if (command_code == iLIST)
            {
            }

            else if (command_code == iCWD)
            {
            }

            else if (command_code == iPWD)
            {
            }
            else if (command_code == QUIT)
            {
            }
        }
    }
    // End of while loop

    close(sockfd);

    return 0;
}

void *handle_user(void *arg)
{
    // collect client info
    thread_parameters *client_info = (thread_parameters *)arg;
    int socket_fd = client_info->cntr_socket;
    struct sockaddr_in *client_addr = &(client_info->address);
    unsigned short current_port = client_info->port;
    char *data_transfer_command = client_info->input;
    int counter_port = client_info->counter;
    int code_command = client_info->command_code;

    // get port
    printf("Parameters in thread: %d %u %s %d %d\n", socket_fd, current_port, data_transfer_command, counter_port, code_command);
    unsigned char p1 = counter_port / 256; // higher byte of port
    unsigned char p2 = counter_port % 256; // lower byte of port
    printf("p1 %i \n", p1);
    printf("p2 %i \n", p2);
    char portInfo[256] = "PORT 127,0,0,1,";

    char portInput[256];
    sprintf(portInput, "%s%i,%i\n", portInfo, p1, p2);
    printf("portInfo:  %s", portInput);

    send(socket_fd, portInput, sizeof(portInfo), 0);

    // send(socket_fd, portInfo, sizeof(portInfo), 0);
    char bufferResponse[1500];
    if (recv(socket_fd, bufferResponse, sizeof(bufferResponse), 0) < 0)
    {
        perror("recv issue, disconnecting");
        return (void*) -1;
    }
    printf("buffer response: %s \n", bufferResponse);

    // get code of response
    int response_code = parse_response(bufferResponse);

    if(response_code == 200) {
        // get socket
        int transfer_sock = socket(AF_INET, SOCK_STREAM, 0);
        if(transfer_sock < 0) {
            perror("Error opening socket");
            return (void*) -1;
        }
        
        // Control connection to the server to port 20 and local host;
        struct sockaddr_in client_address2;
        memset(&client_address2, 0, sizeof(client_address2));
        client_address2.sin_family = AF_INET;
        inet_aton("127.0.0.1", &client_address2.sin_addr);
        client_address2.sin_port = htons(counter_port);

        if (bind(transfer_sock, (struct sockaddr *)&client_address2, sizeof(client_address2)) < 0)
        {
            perror("Bind failed..:");
            return (void*) -1;
        }

        // listen on port
        if (listen(transfer_sock, 5) < 0)
        {
            perror("Listen Error:");
            return (void*) -1;
        }

        bzero(bufferResponse, sizeof(bufferResponse));

        struct sockaddr_in server_address2;
        bzero(&server_address2, sizeof(server_address2));
        int server_sd2 = accept(socket_fd, (struct sockaddr*)&server_address2, sizeof(server_address2));

        printf("New connection from port %d\n", ntohs(server_address2.sin_port));
        
        recv(server_sd2, bufferResponse, sizeof(bufferResponse), 0);
        printf("Received on new connection: %s\n", bufferResponse);
        


    }
    else {
        printf("Command failed: %s\n", data_transfer_command);
    }


    // int dataTransFD = socket(AF_INET, SOCK_STREAM, 0);
    // if (dataTransFD < 0)
    // {
    //     perror("Socket Error!");
    //     exit(1);
    // }
    // // Control connection to the server to port 20 and local host;
    // struct sockaddr_in server_address2;
    // memset(&server_address2, 0, sizeof(server_address2));
    // server_address2.sin_family = AF_INET;
    // inet_aton("127.0.0.1", &server_address2.sin_addr);
    // server_address2.sin_port = htons(2020);

    // if (connect(dataTransFD, (struct sockaddr *)&server_address2, sizeof(server_address2)) < 0)
    // {
    //     perror("connection error");
    //     return -1;
    // }

    if (code_command == STOR)
    {
        printf("STRO! \n");
    }
    else if (code_command == RETR)
    {
        printf("RETR! \n");
    }
    else if (code_command == LIST)
    {
        printf("LIST! \n");
    }
}
