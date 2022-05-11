
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
int ask_server_if_file_exists(int sockfd);
int create_tcp_connection(int port);
int send_new_port(int sockfd, int new_port, char *input);
int create_data_transfer_tcp(struct sockaddr_in *address, unsigned short port);

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
        else if (command_code == QUIT)
        {
            // client closes connection and terminates program
            send(sockfd, input, sizeof(input), 0);
            printf("Connection closed.\n");
            close(sockfd);
            break; // ends while loop
        }

        else if (is_authenticated)
        {

            if (command_code == STOR || command_code == RETR || command_code == LIST)
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
                        // open thread and prepare connection
                        printf("I am about to start thread ... \n ");
                        int td_index = open_thread(busy, sizeof(busy));
                        if (td_index < 0)
                        {
                            printf("Command failed, all threads busy\n");
                        }
                        // set up parameters needed for data transfer
                        thread_parameters data_transfer_info;
                        data_transfer_info.address = server_address;
                        data_transfer_info.port = new_port;
                        data_transfer_info.data = data;
                        data_transfer_info.command_code = command_code;

                        if (pthread_create(thread_ids + td_index, NULL, handle_user, &data_transfer_info) < 0)
                        {
                            perror("Opening thread");
                        }

                        // close any open threads
                        // printf("HERE\n");
                        join_thread(thread_ids, busy, NUM_THREADS);
                        // printf("HERE 2\n");
                    }
                }
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
                if (system("pwd") == -1)
                    printf("Invalid '!pwd' command\n");
            }
        }
    }
    // End of while loop

    close(sockfd);

    return 0;
}

void *handle_user(void *arg)
{
    printf("Now in new thread\n");

    // collect transfer data
    thread_parameters *client_info = (thread_parameters *)arg;
    int command_code = client_info->command_code;
    unsigned short port = client_info->port;
    struct sockaddr_in address = client_info->address;
    char *data = client_info->data;

    printf("w i data %s \n", data);

    // create_data_transfer_tcp
    int transfer_sock = create_data_transfer_tcp(&address, port);

    if (command_code == STOR)
    {
        printf("I am inside store \n");
        send_file(transfer_sock, data);
    }
    else if (command_code == RETR) //
    {
        printf("I am inside retrieve \n");
        receive_file(transfer_sock, data);
    }
    else if (command_code == LIST)
    {
    }
    return 0;
}

int send_new_port(int sockfd, int new_port, char *input)
{
    unsigned char p1 = new_port / 256; // higher byte of port
    unsigned char p2 = new_port % 256; // lower byte of port
    printf("p1 %i \n", p1);
    printf("p2 %i \n", p2);
    char portInfo[256] = "PORT 127,0,0,1,";

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
        return (void *)-1;
    }
    printf("buffer response: %s", bufferResponse);

    // thread flag
    int start_thread = 0;
    // Send actual command after authenticating port
    if (parse_response(bufferResponse) == 200)
    {

        // send command (STOR/RETR/LIST) to server
        printf("Sending: %s\n", input);
        send(sockfd, input, sizeof(input), 0);

        // wait for server response
        bzero(bufferResponse, sizeof(bufferResponse));
        recv(sockfd, bufferResponse, sizeof(bufferResponse), 0);

        printf("Server Response: %s", bufferResponse);

        // Server says file doesn't exist
        if (parse_response(bufferResponse) == 550)
        {
            printf("%s", bufferResponse);
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

int create_data_transfer_tcp(struct sockaddr_in *address, unsigned short port) // I am passing socket as a pointer and it may cause issues
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
    address->sin_port = htons(port);
    if (bind(transfer_sock, (struct sockaddr *)address, sizeof(*address)) < 0)
    {
        perror("Bind failed..:");
        return 0;
    }

    // printf("Here 1\n");

    // listen on port
    if (listen(transfer_sock, 5) < 0)
    {
        perror("Listen Error:");
        return 0;
    }

    struct sockaddr_in server_address;

    // accept new connection from server
    int server_len = sizeof(server_address);                                                               // lent of client cliend address of type sockaddr_in
    int server_sock = accept(transfer_sock, (struct sockaddr *)&server_address, (socklen_t *)&server_len); // accept the connection but also fill the client address with client info
    if (server_sock < 1)
    {
        perror("Accept Error:");
        return 0;
    }

    printf("Port: %d\n", ntohs(server_address.sin_port));
    return transfer_sock;
}
