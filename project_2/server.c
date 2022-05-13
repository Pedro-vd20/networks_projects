#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "constants.h"
#include "user.h"
#include "commands.h"
#include "linked_list.h"
#include "data_transfer.h"


/**
 * @brief maintains control of connection for each client
 *
 * @param arg client information
 */
void handle_user(int usersd, struct sockaddr_in* user_addr);

/**
 * @brief checks if username matches database
 *
 * @param username string to check in db
 * @return int 0 if not authenticated, 1 if found match
 */
int auth_user(char *username);

/**
 * @brief sees if username and password match
 *
 * PREREQ: username has already been matched
 *
 * @param username
 * @param password
 * @return int  0 if no match, 1 if match, -1 if error
 */
int auth_pass(char *username, char *password);

/**
 * @brief Get the port from port command
 * 
 * @param arg info passed by port command in form h1,h2,h3,h4,p1,p2 
 * @param p1 pointer to store p1 in
 * @param p2 pointer to store p2 in
 * @return int return 0 if success, -1 otherwise
 */
int get_port(char* arg, unsigned int* p1, unsigned int* p2);

/**
 * @brief handle active mode transfer/download of a file
 * 
 * @param arg struct with all info needed
 *      port to contact user
 *      command being ran
 *      file name asked (null if not needed)
 * @return void* 
 */
void handle_transfer(unsigned short port, struct sockaddr_in* addr, int command, char* fname);

int main() {
    // printf("Hello\n");

    // socket: create the socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket");
        return -1;
    }

    // printf("Here\n");

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        perror("");
        return -1;
    }

    // bind socket
    struct sockaddr_in server_address;                  // structure to save IP address and port
    memset(&server_address, 0, sizeof(server_address)); // Initialize/fill the server_address to 0
    server_address.sin_family = AF_INET;                // address family
    server_address.sin_port = htons(CTR_PORT);          // port
    inet_aton("127.0.0.1", &server_address.sin_addr);
    if (bind(sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Bind failed..:");
        return -1;
    }

    // printf("Here 1\n");

    // listen on port
    if (listen(sockfd, 5) < 0) {
        perror("Listen Error:");
        return -1;
    }

    struct sockaddr_in client_address; // to store the client information
    bzero(&client_address, sizeof(client_address));

    while (1) {
        int pid = -1;

        // accept new connection from client
        int client_len = sizeof(client_address);                                                      // lent of client cliend address of type sockaddr_in
        int client_sd = accept(sockfd, (struct sockaddr *)&client_address, (socklen_t *)&client_len); // accept the connection but also fill the client address with client info


        if (client_sd < 1) {
            perror("Accept Error:");
            return -1;
        }
        else {
            pid = fork();
        }

        if(pid == 0) {
            handle_user(client_sd, &client_address);
            return 0;
        }
    }

    close(sockfd);
    return 0;
}


void handle_user(int usersd, struct sockaddr_in* user_addr) {

    printf("Connection established with user %d\n", usersd);
    printf("Their port: %d\n", ntohs(user_addr->sin_port));

    // tell user the server is ready
    send(usersd, SERVER_READY, LEN_SERVER_READY, 0);

    // authenticate user
    int auth1 = 0; // flag for username
    int auth2 = 0; // flag for password
    int port_f = 0; // checks if user has already sent port info
    unsigned long port; // holds port info
    char username[50];
    linked_list path; // this prevents a user from accessing data from other users
    init(&path);
    bzero(username, sizeof(username));

    char buffer[256]; // stores received message
    char fname[256];  // stores argument in message
    // listen for messages
    while (1) {
        printf("\n");
        
        bzero(buffer, sizeof(buffer));
        bzero(fname, sizeof(fname));

        // receive command from user
        int bytes = recv(usersd, buffer, sizeof(buffer), 0);
        if (bytes == 0) {
            close(usersd);
            printf("Closed!\n");
            break;
        }

        printf("Received: %s\n", buffer);

        int command = parse_command(buffer, fname);

        printf("Command received: %d\n", command);

        if(command == -1) {
            send(usersd, NOT_IMPLEMENTED, LEN_NOT_IMPLEMENTED, 0);
        }
        else if (command == -2) {
            send(usersd, ERROR, LEN_ERROR, 0);
        }

        // go through possible commands
        // authentication
        else if(command == USER) {
            bzero(username, sizeof(username));
            // if user already logged in, sign out
            auth1 = 0;
            auth2 = 0;
            delete_list(&path);

            // check if username valid
            printf("Username: %s\n", fname);
            auth1 = auth_user(fname);
            printf("%d\n", auth1);
            
            // error reading 
            if(auth1 < 0) {
                send(usersd, ERROR, LEN_ERROR, 0);
                auth1 = 0;
                auth2 = 0;
            }
            // username not matched
            else if (auth1 == 0) {
                // return auth error
                printf("Username not matched\n");
                send(usersd, NOT_LOGGED_IN, LEN_NOT_LOGGED_IN, 0);
                auth2 = 0;
            }
            // user authenticated
            else {
                // username valid
                printf("Successful username verification\n");
                strcpy(username, fname);
                send(usersd, USERNAME_OK, LEN_USERNAME_OK, 0);
            }
        }
        
        // password
        else if(command == PASS) {
            // if no username
            if(!auth1) {
                auth1 = 0;
                auth2 = 0;
                printf("No username provided\n");
                send(usersd, NOT_LOGGED_IN, LEN_NOT_LOGGED_IN, 0);
            }
            // if already authenticated
            else if(auth2) {
                auth1 = 0;
                auth2 = 0;
                delete_list(&path);
                bzero(username, sizeof(username));
                send(usersd, NOT_LOGGED_IN, LEN_NOT_LOGGED_IN, 0);
            }
            else if(!auth2) {
                // check password on csv
                auth2 = auth_pass(username, fname);

                // error reading
                if (auth2 < 0) {
                    send(usersd, ERROR, LEN_ERROR, 0);
                    auth2 = 0;
                    auth1 = 0;
                }

                // not authenticated
                else if(!auth2) {
                    printf("Incorrect password\n");
                    auth1 = 0;
                    send(usersd, NOT_LOGGED_IN, LEN_NOT_LOGGED_IN, 0);
                }
                // login success
                else {
                    printf("Successful login\n");
                    send(usersd, AUTHENTICATED, LEN_AUTHENTICATED, 0);

                    // set up directory info
                    char* p = malloc(256);
                    strcpy(p, username);
                    add_node(&path, p);
                    // printf("HERE\n");
                } 
            }
        }

        // check if not authenticated
        else if(!auth1 && !auth2) {
            printf("Need to sign in first\n");
            send(usersd, NOT_LOGGED_IN, LEN_NOT_LOGGED_IN, 0);
        }

        // all other commands (if this part is reached, user must be authenticated)
        else if(command == PORT) {
            printf("Port command received\n");
            unsigned int p1, p2;

            // collect port
            if(get_port(fname, &p1, &p2) < 0) {
                // error getting port
                printf("Error with port\n");
                send(usersd, ERROR, LEN_ERROR, 0);
                port_f = 0;
            }
            
            else {
                // successfully get port
                port_f = 1;
                port = (p1 * 256) + p2;
                printf("Port: %ld\n", port);
                send(usersd, PORT_SUCCESS, LEN_PORT_SUCCESS, 0);
            }          
        }

        else if(command == STOR || command == RETR || command == LIST) {
            printf("STOR, RETR or LIST\n");

            if(port_f) {
                int fork_f = 1; // used when user uses RETR to check if file exists

                if(command == RETR) {
                    printf("Command is RETR, checking if file exists\n");
                    char* p = get_path(&path);

                    char full_fname[1024];
                    bzero(full_fname, sizeof(full_fname));

                    // construct full file path
                    strcpy(full_fname, p);
                    strcat(full_fname, fname);
                    printf("File to retrieve: %s\n", full_fname);

                    FILE* ptr = fopen(full_fname, "r");
                    if(ptr == NULL) {
                        printf("File not found");
                        send(usersd, NO_SUCH_FILE, LEN_NO_SUCH_FILE, 0);
                        fork_f = 0;
                    }
                    else {
                        fclose(ptr);
                    }

                    free(p);
                }

                // send the okay command
                if(fork_f) {
                    printf("File okay, beginning data connections\n");
                    send(usersd, FILE_OKAY, LEN_FILE_OKAY, 0);

                    // fork process
                    int fid = -1;
                    fid = fork();
                    if(fid == 0) {
                        char* p = get_path(&path);
                        char full_fname[1024];
                        bzero(full_fname, sizeof(full_fname));

                        strcpy(full_fname, p);
                        if(command != LIST) {
                            strcat(full_fname, fname);
                        }
                        printf("Full path requested: %s\n", full_fname);

                        handle_transfer(port, &user_addr, command, full_fname);
                        free(p);
                        return;
                    }
                }

                port_f = 0; // reset port flag for future commands
            }
            else {
                // no port previously specified, error
                printf("Port wasn't previously specified\n");
                send(usersd, NO_PORT, LEN_NO_PORT, 0); // this in theory should never run, as port command is always sent before
            }
        }

        else if(command == CWD) {
            printf("Changing directory to: %s\n", fname);

            // construct new path
            char ls_command[1024];
            bzero(ls_command, sizeof(ls_command));
            
            char* p = get_path(&path);
            strcpy(ls_command, "ls ");
            strcat(ls_command, p);
            strcat(ls_command, fname);

            free(p);

            if(system(ls_command) != 0) {
                send(usersd, ERROR, LEN_ERROR, 0);
            }
            else {
                if(parse_dir(&path, fname, 1) == 0) {
                    print(&path);

                    char response[256];
                    bzero(response, sizeof(response));
                    strcpy(response, CWD_CODE);
                    
                    p = get_path(&path);
                    strcat(response, p);
                    strcat(response, ".\n");
                    free(p);
                    send(usersd, response, strlen(response), 0);
                }
                else {
                    send(usersd, ERROR, LEN_ERROR, 0);
                }
            }
        }
        
        else if(command == PWD) {
            // build path
            char* p = get_path(&path);
            char response[1024];
            bzero(response, sizeof(response));

            strcpy(response, PWD_CODE);
            strcat(response, p);

            send(usersd, response, strlen(response), 0);

            free(p);
        }

        else if(command == QUIT) {
            send(usersd, QUIT_MSG, LEN_QUIT_MSG, 0);
        }
  
    }

    printf("Out of loop!\n");
}

int auth_user(char *username) {
    // open file with usernames
    FILE* fptr = fopen("../users.csv", "r");
    if(fptr == NULL) {
        return -1; // error
    }

    int auth = 0;

    // loop through csv file
    char buffer[256];
    bzero(buffer, sizeof(buffer));
    while (fgets(buffer, sizeof(buffer), fptr) != 0)
    {
        char *user = strtok(buffer, ",");

        if (user == NULL)
        {
            return -1;
        }
        // username found
        else if (strcmp(username, user) == 0)
        {
            auth = 1;
            break;
        }
    }

    fclose(fptr);
    return auth;
}

int auth_pass(char *username, char *password) {
    // open file with usernames
    FILE* fptr = fopen("../users.csv", "r");
    if(fptr == NULL) {
        return -1; // error
    }

    int auth = 0;

    // loop through csv file
    char buffer[256];
    bzero(buffer, sizeof(buffer));
    while (fgets(buffer, sizeof(buffer), fptr) != 0)
    {
        // remove newline
        char *tmp = strtok(buffer, "\n");

        char *token = strtok(tmp, ",");
        if (token == NULL)
        {
            return -1;
        }
        // username found
        else if (strcmp(username, token) == 0)
        {
            // get password
            token = strtok(NULL, ",");
            if (token == NULL)
            {
                return -1;
            }

            auth = (strcmp(password, token) == 0);
            break;
        }
    }

    fclose(fptr);
    return auth;
}

int get_port(char* arg, unsigned int* p1, unsigned int* p2) {
    // h1,h2,h3,h4,p1,p2
    printf("Port received: %s\n", arg);

    // go through the string by commas
    char* token = strtok(arg, ","); // h1
    if(token == NULL) {
        return -1;
    }

    // go through h2-h4
    for(int i = 0; i < 4; ++i) {
        token = strtok(NULL, ",");
        if(token == NULL) {
            return -1;
        }
    }

    // token now points to p1
    *p1 = atoi(token);

    // get p2
    token = strtok(NULL, ",");
    if(token == NULL) {
        return -1;
    }
    *p2 = atoi(token);

    return 0;
}

void handle_transfer(unsigned short port, struct sockaddr_in* addr, int command, char* fname) {

    printf("Now in thread\n");
    
    // printf("HERE \n");
    
    int transfer_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(transfer_sock < 0) {
        perror("Socket");
        return;
    }

    // printf("HERE 2\n");

    //setsock
	setsockopt(transfer_sock, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)); 
	struct sockaddr_in client_addr;
	bzero(&client_addr,sizeof(client_addr));
	client_addr.sin_family = AF_INET;
	client_addr.sin_port = htons(port);
	client_addr.sin_addr.s_addr = addr->sin_addr.s_addr; 

    // printf("HERE 3\n");

    printf("Set up client addess\n");

	//Following code will specify the local port number (20) which will be 
	//used for the connection to the remote machine (e.g. ftp client). 
	struct sockaddr_in my_addr;
	bzero(&my_addr,sizeof(my_addr));
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(TRANSFER_PORT);
	if(bind(transfer_sock, (struct sockaddr *)(&my_addr),sizeof(my_addr)) < 0) {
        perror("Binding");
        return;
    }
    
    printf("Connecting to user\n");

    // connect to client
    while (connect(transfer_sock, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {

    }

    printf("Connection success\n");

    // once connection established
    if(command == RETR) {
        printf("Sending file\n");

        send_file(transfer_sock, fname);
    }
    else if(command == STOR) {
        printf("Storing file\n");

        receive_file(transfer_sock, fname);
    }
    else {
        printf("Listing directory\n");

        
    }

    close(transfer_sock);

}