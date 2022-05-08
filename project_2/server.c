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
#include "threading.h"


/**
 * @brief maintains control of connection for each client
 * 
 * @param arg client information
 */
void* handle_user(void* arg);

/**
 * @brief checks if username matches database
 * 
 * @param username string to check in db
 * @return int 0 if not authenticated, 1 if found match
 */
int auth_user(char* username);

/**
 * @brief sees if username and password match
 * 
 * PREREQ: username has already been matched
 * 
 * @param username 
 * @param password 
 * @return int  0 if no match, 1 if match, -1 if error
 */
int auth_pass(char* username, char* password);

/**
 * @brief Get the port from port command
 * 
 * @param arg info passed by port command in form h1,h2,h3,h4,p1,p2 
 * @param p1 pointer to store p1 in
 * @param p2 pointer to store p2 in
 * @return int return 0 if success, -1 otherwise
 */
int get_port(char* arg, unsigned int* p1, unsigned int* p2);

int main() {
    // printf("Hello\n");
    
    // socket: create the socket 
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket");
        return -1;
    }

    // printf("Here\n");

    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        perror("");
        return -1;
    }
    
    // bind socket
	struct sockaddr_in server_address;	//structure to save IP address and port
	memset(&server_address,0,sizeof(server_address)); //Initialize/fill the server_address to 0
	server_address.sin_family = AF_INET;	//address family
	server_address.sin_port = htons(CTR_PORT);	//port
	inet_aton("127.0.0.1", &server_address.sin_addr);
	if(bind(sockfd,(struct sockaddr *)&server_address,sizeof(server_address))<0) {
		perror("Bind failed..:");
		return -1;
	}

    // printf("Here 1\n");

	// listen on port
	if(listen(sockfd,5)<0) {
		perror("Listen Error:");
		return -1;
	}

	struct sockaddr_in client_address;  //to store the client information

	// multithread addresses to deal with requests
	pthread_t thread_ids[NUM_THREADS];
	int busy[NUM_THREADS]; // keep track of threads currently in use
	bzero(busy, sizeof(busy)); 

    while(1) {
        // accept new connection from client
		int client_len = sizeof(client_address);	//lent of client cliend address of type sockaddr_in
		int client_sd = accept(sockfd,(struct sockaddr*)&client_address,(socklen_t *)&client_len);	//accept the connection but also fill the client address with client info
		if(client_sd<1)
		{
			perror("Accept Error:");
			return -1;
		}

		// send thread to handle response
		client user;
		user.socket = client_sd;
		user.address = client_address;
		int t_id_index = open_thread(busy, NUM_THREADS);
		// check if index -1 (figure out how to handle later)
		if(pthread_create(thread_ids + t_id_index, NULL, handle_user, &user) < 0) {
			perror("multithreading");
			return -1;
		}

		// close threads that finish running
		join_thread(thread_ids, busy, NUM_THREADS);
	
    }

    close(sockfd);
    return 0;
}


void* handle_user(void* arg) {
    // SERVER IS SUPPOSED TO SEND A 220 FIRST!!!!!!
    
    // collect client info
    client* user = (client*) arg;
    int usersd = user->socket;
    struct sockaddr_in* user_addr = &(user->address);

    printf("Connection established with user %d\n", usersd);

    // authenticate user
    int auth1 = 0; // flag for username
    int auth2 = 0; // flag for password
    int port_f = 0; // checks if user has already sent port info
    unsigned long port; // holds port info
    char username[50];
    bzero(username, sizeof(username));

    char buffer[256]; // stores received message
    char fname[256]; // stores argument in message 
    // listen for messages
    while(1) {
        bzero(buffer, sizeof(buffer));
        bzero(fname, sizeof(fname)); 

        // receive command from user
        int bytes = recv(usersd, buffer, sizeof(buffer), 0);
        if(bytes == 0) {
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
        else if(command == -2) {
            send(usersd, ERROR, LEN_ERROR, 0);
        }
        
        // authenticate user
        if(!(auth1 && auth2)) {
            
            // check for receiving username
            if(!auth1 && command == USER) {
                printf("Username: %s\n", fname);
                auth1 = auth_user(fname);
                printf("%d\n", auth1);
                // error reading 
                if(auth1 < 0) {
                    send(usersd, ERROR, LEN_ERROR, 0);
                }
                else if(!auth1) {
                    // return auth error
                    send(usersd, NOT_LOGGED_IN, LEN_NOT_LOGGED_IN, 0);
                }
                else {
                    // username valid
                    strcpy(username, fname);
                    send(usersd, USERNAME_OK, LEN_USERNAME_OK, 0);
                }
            }

            // check for receiving password
            else if(auth1 && !auth2 && command == PASS) {
                auth2 = auth_pass(username, fname);
                // error reading
                if(auth2 < 0) {
                    send(usersd, ERROR, LEN_ERROR, 0);
                }
                // not authenticated
                else if(!auth2) {
                    send(usersd, NOT_LOGGED_IN, LEN_NOT_LOGGED_IN, 0);
                }
                // login success
                else {
                    send(usersd, AUTHENTICATED, LEN_AUTHENTICATED, 0);
                }
            }
            // Not logged in / logging in
            else {
                send(usersd, NOT_LOGGED_IN, LEN_NOT_LOGGED_IN, 0);
            }

            continue;
        }
        else {
            
            // go through all the commands
            switch (command)
            {
            case PORT:
                printf("Port command received\n");
                unsigned int p1, p2;
                if(get_port(fname, &p1, &p2) < 0) {
                    // error getting port
                    printf("HERE\n");
                    send(usersd, ERROR, LEN_ERROR, 0);
                }
                else {
                    port_f = 1;
                    port = (p1 * 256) + p2;
                    printf("Port: %ld\n", port);
                }
                break;
            
            default:
                break;
            }
        }
    

    }
}

int auth_user(char* username) {
    // open file with usernames
    FILE* fptr = fopen("../users.csv", "r");
    if(fptr == NULL) {
        return -1; // error
    }

    int auth = 0;
    
    // loop through csv file
    char buffer[256];
    bzero(buffer, sizeof(buffer));
    while(fgets(buffer, sizeof(buffer), fptr) != 0) {
        char* user = strtok(buffer, ",");
        if(user == NULL) {
            return -1;
        }
        // username found
        else if(strcmp(username, user) == 0) {
            auth = 1;
            break;
        }
    }


    fclose(fptr);
    return auth;
}

int auth_pass(char* username, char* password) {
    // open file with usernames
    FILE* fptr = fopen("../users.csv", "r");
    if(fptr == NULL) {
        return -1; // error
    }

    int auth = 0;
    
    // loop through csv file
    char buffer[256];
    bzero(buffer, sizeof(buffer));
    while(fgets(buffer, sizeof(buffer), fptr) != 0) {
        // remove newline
        char* tmp = strtok(buffer, "\n");
        
        char* token = strtok(tmp, ",");
        if(token == NULL) {
            return -1;
        }
        // username found
        else if(strcmp(username, token) == 0) {
            // get password
            token = strtok(NULL, ",");
            if(token == NULL) {
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

// int main() {

    /*
    Bind socket

    Start listening port 21

    multithread response

        listen for user commands

        Check user commands (and authenticate)

        Perform action

        Return message

        IF FILE TRANSFER

            Start new thread

            Receive user port

            Start comm with port 20

            Send file    

            Close port

            Close thread
        
        IF END COMM

            close port

            close thread

    */


    // return 0;
// }