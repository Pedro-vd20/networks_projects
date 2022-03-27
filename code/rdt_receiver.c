#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <assert.h>

#include "common.h"
#include "packet.h"
#include "linked_list.h"


/*
 * You are required to change the implementation to support
 * window size greater than one.
 * In the current implementation the window size is one, hence we have
 * only one send and receive packet
 */
tcp_packet *recvpkt;
tcp_packet *sndpkt;
linked_list packets;
int expec_seqno = 0;

void add_pkt_to_list(){

    if(is_empty(&packets)){
        add_node(&packets, recvpkt);
        return;
    }

    if(recvpkt->hdr.seqno < packets.head->p->hdr.seqno){
        struct node* newHead = malloc(sizeof(struct node));
        newHead->p = recvpkt; 
        newHead->prev = NULL;
        newHead->next = packets.head;
        packets.head = newHead;
        return;
    }

    int isDuplicate = 0;
    struct node* temp_pointer = packets.head;
    while(temp_pointer->next != NULL && temp_pointer->next->p->hdr.seqno < recvpkt->hdr.seqno){
        if(temp_pointer->p->hdr.seqno == recvpkt->hdr.seqno) isDuplicate = 1; 
        temp_pointer = temp_pointer -> next;
    }

    if(isDuplicate == 0){
        struct node* newNode = malloc(sizeof(struct node));
        newNode -> p = recvpkt;
        newNode -> prev = temp_pointer;
        newNode -> next = temp_pointer -> next;
        temp_pointer->next = newNode;
        temp_pointer -> next -> prev = newNode;
    } 
        
    return;
}

void write_to_file(FILE *fp){ 
    while(!is_empty(&packets)) {
        tcp_packet *head_pkt = get_head(&packets);
        if(expec_seqno != head_pkt -> hdr.seqno) {
            break;
        }
        fseek(fp, recvpkt->hdr.seqno, SEEK_SET); 
        fwrite(recvpkt->data, 1, recvpkt->hdr.data_size, fp); //write on file
        expec_seqno = recvpkt->hdr.seqno + recvpkt->hdr.data_size; //update the expected seqno
        remove_node(&packets, 1);
    }
    return;
}

int main(int argc, char **argv) {
    int sockfd; /* socket */
    int portno; /* port to listen on */
    int clientlen; /* byte size of client's address */
    struct sockaddr_in serveraddr; /* server's addr */
    struct sockaddr_in clientaddr; /* client addr */
    int optval; /* flag value for setsockopt */
    FILE *fp;
    char buffer[MSS_SIZE];
    struct timeval tp;
     //expected sequence number

    /* 
     * check command line arguments 
     */
    if (argc != 3) {
        fprintf(stderr, "usage: %s <port> FILE_RECVD\n", argv[0]);
        exit(1);
    }
    portno = atoi(argv[1]);

    fp  = fopen(argv[2], "w");
    if (fp == NULL) {
        error(argv[2]);
    }

    /* 
     * socket: create the parent socket 
     */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    /* setsockopt: Handy debugging trick that lets 
     * us rerun the server immediately after we kill it; 
     * otherwise we have to wait about 20 secs. 
     * Eliminates "ERROR on binding: Address already in use" error. 
     */
    optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
            (const void *)&optval , sizeof(int));

    /*
     * build the server's Internet address
     */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)portno);

    /* 
     * bind: associate the parent socket with a port 
     */
    if (bind(sockfd, (struct sockaddr *) &serveraddr, 
                sizeof(serveraddr)) < 0) 
        error("ERROR on binding");

    /* 
     * main loop: wait for a datagram, then echo it
     */
    VLOG(DEBUG, "epoch time, bytes received, sequence number");
    int sendAckno = 1; 
    clientlen = sizeof(clientaddr);
    
    while (1) {
        /*
         * recvfrom: receive a UDP datagram from a client
         */
        //VLOG(DEBUG, "waiting from server \n");
        if (recvfrom(sockfd, buffer, MSS_SIZE, 0,
                (struct sockaddr *) &clientaddr, (socklen_t *)&clientlen) < 0) {
            error("ERROR in recvfrom");
        }
        recvpkt = (tcp_packet *) buffer;
        assert(get_data_size(recvpkt) <= DATA_SIZE);
        if ( recvpkt->hdr.data_size == 0) {
            //VLOG(INFO, "End Of File has been reached");
            fclose(fp);
            break;
        }
        /* 
         * sendto: ACK back to the client 
         */
        gettimeofday(&tp, NULL);
        VLOG(DEBUG, "%lu, %d, %d", tp.tv_sec, recvpkt->hdr.data_size, recvpkt->hdr.seqno);

         if (recvpkt->hdr.seqno == expec_seqno){ //Expected package
            //Add to LinkedList
            add_pkt_to_list();
            write_to_file(fp);
            sendAckno = 1; //Send Acknoledgement
	    }
        else if(recvpkt->hdr.seqno < expec_seqno){ //If it is less than it is a duplicate
            // printf("duplicate \n");
            sendAckno = 1; //Resend Acknoledgement
        } 
        else if (recvpkt->hdr.seqno > expec_seqno){
            // printf("out of order \n");
            write_to_file(fp);
            print(&packets);
            sendAckno = 0; //Do not Send Acknoledgement
	    }
        //Send acknowledgement if there is a duplicate or we got the expected package
        if(sendAckno) {
            sndpkt = make_packet(0);
            sndpkt->hdr.ackno =  expec_seqno;
            sndpkt->hdr.ctr_flags = ACK;
            if (sendto(sockfd, sndpkt, TCP_HDR_SIZE, 0, (struct sockaddr *) &clientaddr, clientlen) < 0) {
                error("ERROR in sendto");
            }
        }    
    }
    print(&packets);

    return 0;
}
