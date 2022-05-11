
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
#include "data_transfer.h"

#define USERNAME_OK "331 Username OK, need password\n"
#define AUTHENTICATED "230 User logged in, proceed\n"
#define NOT_LOGGED_IN "530 Not logged in\n"
#define ERROR "501 Syntax error in parameters or arguments\n"
// -1  command not identified
//  *      -2 syntax error
//  *       0  PORT
#define PORT 0
#define USER 1
#define PASS 2
#define STOR 3
#define RETR 4
#define LIST 5
#define iLIST 6
#define CWD 7
#define iCWD 8
#define PWD 9
#define iPWD 10
#define QUIT 11

/**
 * @brief maintains control of connection for each client
 *
 * @param arg client information
 */
void *handle_user(void *arg);
int ask_server_if_file_exists(int sockfd);
int create_tcp_connection(int port);
int send_new_port(int sockfd, int port);

int main(int argc, char **argv)
{

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

    printf("[%s:%i] > \n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));

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
    int port_counter = 0;
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
                data_transfer_info.control_socket = sockfd;
                data_transfer_info.address = server_address;
                data_transfer_info.port = control_port;
                data_transfer_info.input = input;
                data_transfer_info.counter = port_counter;
                data_transfer_info.command_code = command_code;
                data_transfer_info.data = data;
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

    // --------------------------Handling Parameters
    thread_parameters *client_info = (thread_parameters *)arg;
    int socket_fd = client_info->control_socket;
    struct sockaddr_in *client_addr = &(client_info->address);
    unsigned short current_port = client_info->port;
    char *data_transfer_command = client_info->input;
    char *filename = client_info->data;
    int counter_port = client_info->counter;
    int code_command = client_info->command_code;

    // ------------------------Send new client port to server and handle its response
    int new_port = current_port + counter_port;
    int was_port_accepted = send_new_port(socket_fd, new_port); // Should print "200 PORT command successful."
    if (!was_port_accepted)
        return 0; // Go back to main

    //-------------------------Handle each data transfer command uniquely
    int data_sockfd;
    if (code_command == STOR)
    {
        // 1. Check if file exists

        data_sockfd = create_tcp_connection(new_port);
        if (data_sockfd < 0)
            return 0;

        // 3. Upload file
        send_file(data_sockfd, filename);
    }
    else if (code_command == RETR) //
    {
        printf("RETR! \n"); // delete
        // 1.  Check if the file exists in the server
        if (ask_server_if_file_exists(socket_fd) == 0) // If file does not exist in the server then exit thread
        {
            printf("file does not exist \n");
            return 0;
        }

        // 2. If the file does exist then connect to a new tcp data conection
        data_sockfd = create_tcp_connection(new_port);
        if (data_sockfd < 0)
            return 0;

        // 3. dowload_file from server
        receive_file(data_sockfd, filename);
    }
    else if (code_command == LIST)
    {
        data_sockfd = create_tcp_connection(new_port);
        if (data_sockfd < 0)
            return 0;

        printf("LIST! \n");
    }
    return 0;
}

int create_tcp_connection(int port)

{
    printf("Starting create_tcp \n");
    int dataTransFD = socket(AF_INET, SOCK_STREAM, 0);
    if (dataTransFD < 0)
    {
        perror("Socket Error!");
        return 0;
    }

    // Declare Local client address to bind port number
    struct sockaddr_in client_address;
    memset(&client_address, 0, sizeof(client_address));
    client_address.sin_family = AF_INET;
    inet_aton("127.0.0.1", &client_address.sin_addr);
    client_address.sin_port = htons(port);

    if (bind(dataTransFD, (struct sockaddr *)&client_address, sizeof(client_address)) < 0)
    {
        perror("error binding... \n");
        return 0;
    }

    // Control connection to the server to port 20 and local host;
    struct sockaddr_in server_address2;
    memset(&server_address2, 0, sizeof(server_address2));
    server_address2.sin_family = AF_INET;
    inet_aton("127.0.0.1", &server_address2.sin_addr);
    server_address2.sin_port = htons(2020);

    if (connect(dataTransFD, (struct sockaddr *)&server_address2, sizeof(server_address2)) < 0)
    {
        perror("connection error");
        return 0;
    }
    printf("Ending creating tcp \n");
    return dataTransFD;
}

int send_new_port(int sockfd, int port)
{
    printf("starting send new port \n");
    unsigned char p1 = port / 256; // higher byte of port
    unsigned char p2 = port % 256; // lower byte of port
    printf("p1 %i \n ", p1);       // delete
    printf("p2 %i \n", p2);        // delete
    char portInfo[256] = "PORT 127,0,0,1,";

    char portInput[256];
    sprintf(portInput, "%s%i,%i", portInfo, p1, p2);
    printf("portInfo:  %s \n", portInput); // delete

    char bufferResponse[256];

    int was_succesful;
    while (1)
    {
        send(sockfd, portInput, sizeof(portInput), 0);

        if (recv(sockfd, bufferResponse, sizeof(bufferResponse), 0) < 0)
        {
            perror("recv issue, disconnecting");
            break;
        }
        // parse buffer Response;
        printf("parse_response %d \n", parse_response(bufferResponse));
        if (parse_response(bufferResponse) == 200)
        {
            printf("buffer response: %s \n", bufferResponse); // This should print "200 PORT command successful."
            was_succesful = 1;
        }
        else
        {
            printf("buffer response: %s \n", bufferResponse);
            was_succesful = 0;
        }
        break;
    }
    printf("finishing sending new port \n");
    return was_succesful;
}

int ask_server_if_file_exists(int sockfd)
{

    printf("starting ask server \n");
    char bufferResponse[256];

    int does_file_exist;
    while (1)
    {
        // send(sockfd, sizeof(portInput), 0);

        if (recv(sockfd, bufferResponse, sizeof(bufferResponse), 0) < 0)
        {
            perror("recv issue, disconnecting");
            break;
        }
        // parse buffer Response;
        printf("parse_response %d \n", parse_response(bufferResponse));
        if (parse_response(bufferResponse) == 150)
        {
            does_file_exist = 1;
        }
        else
        {

            does_file_exist = 0;
        }
        printf("%s \n", bufferResponse); // This should print "200 PORT command successful."
        break;
        // To improve while is not that well designed
    }
    printf("ending ask server \n");
    return does_file_exist;
}