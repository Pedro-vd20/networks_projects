
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <stdlib.h>
#include "commands.h"
#include "constants.h"
#include "user.h"
#include "data_transfer.h"

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
 * @brief maintains control of connection for each transfer. Concurrent Transfer Implement
 *
 * @param arg client information
 */
void handle_transfer(unsigned short port, struct sockaddr_in address, int command_code, char *data);

/**
 * @brief Makes port calculation and send  to the server the port in which the client will listen
 *
 * @param sockfd control socket
 * @param new_port N + i port, where client will listen
 * @param
 */
int send_new_port(int sockfd, int new_port, char *input);

int main(int argc, char **argv)
{

    // // Declare and verify socket file descriptor
    // linked_list path;
    // init(&path);
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

    printf("[%s:%i] > \n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));

    unsigned short control_port = ntohs(client.sin_port);
    // User Interface
    printf("control socket connected!\n");
    // menu
    printf("Hello!! Please Authenticate to run server commands  \n");
    printf("1. type \"USER\" followed by a space and your username \n");
    printf("2. type \"PASS\" followed by a space and your password \n");
    printf("\n");
    printf("\"QUIT\" to close connection at any moment\n");
    printf("Once Authenticated \n");
    printf("this is the list of commands : \n");
    printf("\"STOR\" + space + filename |to send a file to the server \n");
    printf("\"RETR\" + space + filename |to download a file from the server \n");
    printf("\"LIST\" |to  to list all the files under the current server directory \n");
    printf("\"CWD\" + space + directory |to change the current server directory \n");
    printf("\"PWD\" to display the current server directory \n");

    printf("Add \"!\" before the last three commands to apply them locally \n\n");

    int port_counter = 1;
    char first_buffer[256];
    bzero(first_buffer, 256);
    recv(sockfd, first_buffer, 256, 0);
    printf("%s", first_buffer);

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

        if (command_code == USER)
        {

            send(sockfd, input, sizeof(input), 0);
            bzero(input, sizeof(input));
            if (recv(sockfd, response, sizeof(response), 0) < 0)
            {
                perror("recv error!!");
                break;
            }

            printf("%s", response);
        }
        else if (command_code == PASS)
        {
            send(sockfd, input, sizeof(input), 0);
            bzero(response, sizeof(response));
            if (recv(sockfd, response, sizeof(response), 0) < 0)
            {
                perror("recv error!!");
                break;
            }

            printf("%s", response);
        }
        else if (command_code == QUIT)
        {
            // client closes connection and terminates program
            send(sockfd, input, sizeof(input), 0);

            bzero(response, sizeof(response));
            if (recv(sockfd, response, sizeof(response), 0) < 0)
            {
                perror("recv error!!");
                break;
            }

            printf("%s", response);

            if (parse_response(response) == 221)
            {
                close(sockfd);
                break; // ends while loop
            }
            else
            {
                printf("Error sending QUIT command\n");
            }
        }
        else if (command_code == STOR || command_code == RETR || command_code == LIST)
        {
            // flag (only relevant for STOR)
            int f_exists = 1;
            if (command_code == STOR)
            {
                FILE *ptr = fopen(data, "r");
                printf("File to send: %s\n", data);

                // check if file exists
                if (ptr == NULL)
                {
                    f_exists = 0;
                    printf("%s", NO_SUCH_FILE);
                }
                else
                {
                    fclose(ptr);
                }
            }

            if (f_exists)
            {

                unsigned short new_port = control_port + port_counter++;

                // set up port
                int start_thread = send_new_port(sockfd, new_port, input);
                // if command successfully set up, start parallel connection
                if (start_thread)
                {

                    int pid = -1;

                    pid = fork();
                    if (pid == 0)
                    {
                        handle_transfer(new_port, server_address, command_code, data);
                        return 0;
                    }
                }
            }
        }

        else if (command_code == PWD || command_code == CWD)
        {
            send(sockfd, input, sizeof(input), 0);
            char bufferResponse[1500];
            bzero(bufferResponse, sizeof(bufferResponse));
            if (recv(sockfd, bufferResponse, sizeof(bufferResponse), 0) < 0)
            {
                perror("recv issue, disconnecting");
                break;
            }
            printf("%s\n", bufferResponse);
        }
        else if (command_code == iLIST)
        {

            if (system("ls") == -1)
                printf("Invalid '!ls' command\n");
        }

        else if (command_code == iCWD)
        {

            if (chdir(data) == -1)
                printf("Invalid '!cwd' command\n");
            else
                printf("Local directory successfully changed.\n");
        }

        else if (command_code == iPWD)
        {

            if (system("pwd") == -1)
                printf("Invalid '!pwd' command\n");
        }
    }
    // End of while loop

    close(sockfd);

    return 0;
}

void handle_transfer(unsigned short port, struct sockaddr_in address, int command_code, char *data)
{
    printf("Now in new thread\n");

    printf("w i data %s \n", data);

    // open socket
    int transfer_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (transfer_sock < 0)
    {
        perror("Socket");
        return;
    }

    // bind socket
    if (setsockopt(transfer_sock, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
    {
        perror("binding faield! \n");
        return;
    }

    // bind socket
    printf("Port: %d\n", port);

    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_aton("127.0.0.1", &(addr.sin_addr));
    if (bind(transfer_sock, (struct sockaddr *)&addr, sizeof(address)) < 0)
    {
        perror("Bind failed..:");
        return;
    }

    // printf("Here 1\n");

    // listen on port
    printf("about to listen... \n");
    if (listen(transfer_sock, 5) < 0)
    {
        perror("Listen Error:");
        return;
    }
    printf("after listening... \n"); //##########DELETE

    struct sockaddr_in server_address;
    bzero(&server_address, sizeof(server_address));

    // accept new connection from server
    int server_len = sizeof(server_address);                                                               // lent of client cliend address of type sockaddr_in
    printf("before accepting ...\n");                                                                      //#########DELETE
    int server_sock = accept(transfer_sock, (struct sockaddr *)&server_address, (socklen_t *)&server_len); // accept the connection but also fill the client address with client info
    if (server_sock < 1)
    {
        perror("Accept Error:");
        return;
    }

    printf("Port: %d\n", ntohs(server_address.sin_port));

    if (command_code == STOR)
    {
        printf("I am inside store \n");
        send_file(server_sock, data);
        close(server_sock);
        close(transfer_sock);
    }
    else if (command_code == RETR) //
    {
        printf("I am inside retrieve \n");
        receive_file(server_sock, data);
    }
    else if (command_code == LIST)
    {
        char bufferResponse[1024]; // buffr to store response

        while (1)
        {
            bzero(bufferResponse, sizeof(bufferResponse)); // clean buffer
            // wait for server to send a buffer with the list of files/directories
            if (recv(server_sock, bufferResponse, sizeof(bufferResponse), 0) < 0)
            {
                printf("Nothing sent\n");
                close(transfer_sock);
                return;
            }

            printf("%s\n", bufferResponse);

            if (strcmp(bufferResponse, TRANSFER_COMPLETE) == 0)
            {
                close(transfer_sock);
                break;
            }
        }
    }
}

int send_new_port(int sockfd, int new_port, char *input)
{
    unsigned char p1 = new_port / 256; // higher byte of port
    unsigned char p2 = new_port % 256; // lower byte of port
    printf("p1 %i \n", p1);
    printf("p2 %i \n", p2);
    // Temporal buffer needed for string manipulation
    char portInfo[256] = "PORT 127,0,0,1,";

    // buffer that will be sent to the server
    char portInput[256];
    sprintf(portInput, "%s%i,%i\n", portInfo, p1, p2);
    printf("portInfo:  %s", portInput);

    // Send PORT to server
    send(sockfd, portInput, sizeof(portInput), 0);

    // Receive port confirmation
    char bufferResponse[1500];
    bzero(bufferResponse, sizeof(bufferResponse));
    if (recv(sockfd, bufferResponse, sizeof(bufferResponse), 0) < 0)
    {
        perror("recv issue, disconnecting");
        return 0;
    }
    printf("buffer response: %s", bufferResponse);

    // thread flag
    int start_thread = 0;
    // Send actual command after authenticating port
    if (parse_response(bufferResponse) == 200)
    {
        // buffer that stores the full input of the user
        char user_input[256];
        strcpy(user_input, input);
        // send command (STOR/RETR/LIST) to server
        printf("Sending: %s\n", user_input);

        send(sockfd, user_input, sizeof(user_input), 0);

        // wait for server response
        bzero(bufferResponse, sizeof(bufferResponse));
        recv(sockfd, bufferResponse, sizeof(bufferResponse), 0);

        printf("Server Response: %s", bufferResponse);

        // Server says file doesn't exist
        if (parse_response(bufferResponse) == 550)
        {
            printf("%s \n", bufferResponse);
        }
        // server says file okay
        else if (parse_response(bufferResponse) == 150)
        {
            printf("start thread = 1 \n");
            start_thread = 1;
        }
    }
    else
    {
        printf("%s\nError setting port up\n", bufferResponse);
    }

    return start_thread;
}

int create_data_transfer_tcp(struct sockaddr_in *address, unsigned short port, int command, char *data) // I am passing socket as a pointer and it may cause issues
{
    // open socket
    int transfer_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (transfer_sock < 0)
    {
        perror("Socket");
        return 0;
    }

    // bind socket
    if (setsockopt(transfer_sock, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
    {
        perror("binding faield! \n");
        return 0;
    }

    // bind socket
    printf("Port: %d\n", port);
    address->sin_port = htons(port);
    if (bind(transfer_sock, (struct sockaddr *)address, sizeof(*address)) < 0)
    {
        perror("Bind failed..:");
        return 0;
    }

    // printf("Here 1\n");

    // listen on port
    printf("about to listen... \n");
    if (listen(transfer_sock, 5) < 0)
    {
        perror("Listen Error:");
        return 0;
    }
    printf("after listening... \n"); //##########DELETE

    struct sockaddr_in server_address;

    // accept new connection from server
    int server_len = sizeof(server_address);                                                               // lent of client cliend address of type sockaddr_in
    printf("before accepting ...\n");                                                                      //#########DELETE
    int server_sock = accept(transfer_sock, (struct sockaddr *)&server_address, (socklen_t *)&server_len); // accept the connection but also fill the client address with client info
    if (server_sock < 1)
    {
        perror("Accept Error:");
        return 0;
    }

    printf("Port: %d\n", ntohs(server_address.sin_port));

    if (command)

        return transfer_sock;
}
