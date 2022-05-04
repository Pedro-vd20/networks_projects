
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

    while (1)
    {
        char input[50] = "\0";
        fgets(input, 50, stdin);
        input[strcspn(input, "\n")] = 0;

        char copy_input[50];
        char data[40];
        strcpy(copy_input, input);

        int command_code = parse_command(copy_input, data);

        if (command_code == -1)
        {
            printf("Command Not Found ! \n");
        }
        else if (command_code == -2)
        {
            printf("Syntax Error \n");
        }

        char response[100];

        if (command_code == 1 && strlen(data) > 0)
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
                printf("331 Username OK, need password.\n");
            }
            else
            {
                printf("Username not registered! \n");
            }
        }
        else if (command_code == 2 && strlen(data) > 0 && is_username_ok)
        {
            send(sockfd, input, sizeof(input), 0);
            if (recv(sockfd, response, sizeof(response), 0) < 0)
            {
                perror("recv error!!");
                break;
            }

            if (parse_response(response) == 230)
            {
                printf("230 User logged in, proceed. \n");
                is_authenticated = 1;
            }
            else if (parse_response(response) == 530)
            {
                printf("530 Not logged in. \n");
            }
        }
        else if (is_authenticated)
        {
            printf("I got authenticated!! \n");
        }

    } // End of while loop

    close(sockfd);

    // User I
    /*
        Bind stuff

        Comm to port 21

        Authenticate user

        Loop asking user

            Ask user for command

            Check command validity

            Send command

            Await response

            Display response

            IF FILE TRANSFER REQUEST

                Start thread

                Send server new random port

                Listen on that port for server (listen for port 20)

                Close port

                Close thread



    */

    return 0;
}

void *handle_request(void);