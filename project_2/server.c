#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "user.h"
#include "threading.h"

/**
 * @brief maintains control connection for each client 
 * 
 * @return void* client information 
 */
void* handle_user(void*);

int main(int argc, char **argv) {
    /* socket: create the socket */
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket");
        return -1;
    }

    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        perror("");
        return -1;
    }
    
    // bind socket
	struct sockaddr_in server_address;	//structure to save IP address and port
	memset(&server_address,0,sizeof(server_address)); //Initialize/fill the server_address to 0
	server_address.sin_family = AF_INET;	//address family
	server_address.sin_port = htons(2021);	//port
	inet_aton("127.0.0.1", &server_address.sin_addr);
	if(bind(sockfd,(struct sockaddr *)&server_address,sizeof(server_address))<0) {
		perror("Bind failed..:");
		return -1;
	}

	// listen on port
	if(listen(sockfd,5)<0) {
		perror("Listen Error:");
		return -1;
	}

	struct sockaddr_in client_address;  //to store the client information

	// multithread addresses to deal with requests
	pthread_t thread_ids[10];
	int busy[10]; // keep track of threads currently in use
	bzero(busy, sizeof(busy)); 

    while(1) {
        // accept new connection from client
		int client_len=sizeof(client_address);	//lent of client cliend address of type sockaddr_in
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
		int t_id_index = open_thread(busy, 10);
		// check if index -1 (figure out how to handle later)
		if(pthread_create(thread_ids + t_id_index, NULL, handle_user, &user) < 0) {
			perror("multithreading");
			return -1;
		}

		// close threads that finish running
		// join_thread(thread_ids, busy, 10);
	
    }

    close(sockfd);
    return 0;
}

void* handle_user(void* arg) {
    // collect client info
    client* user = (client*) arg;

    printf("Connection from: %d\n", user->socket);
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