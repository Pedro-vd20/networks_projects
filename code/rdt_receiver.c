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
    tcp_packet* cpy_recvpkt = make_packet(recvpkt -> hdr.data_size);
    memcpy(cpy_recvpkt->data, recvpkt->data, recvpkt->hdr.data_size);
    cpy_recvpkt->hdr.ackno = recvpkt->hdr.ackno;
    cpy_recvpkt->hdr.seqno = recvpkt->hdr.seqno;

    if(is_empty(&packets)){
        add_node(&packets, cpy_recvpkt);
        return;
    }

    
    if(cpy_recvpkt->hdr.seqno < packets.head->p->hdr.seqno){
        
        struct node* newHead = malloc(sizeof(struct node));
        newHead->p = cpy_recvpkt; 
        newHead->prev = NULL;
        newHead->next = packets.head;
        packets.head = newHead;
        packets.size++;
        return;
    }
    
    
    if(cpy_recvpkt->hdr.seqno > packets.tail->p->hdr.seqno){
    
        struct node* newTail = malloc(sizeof(struct node));
        newTail->p = cpy_recvpkt;
        newTail->prev = packets.tail;
        packets.tail->next = newTail;
        newTail->next = NULL;
        packets.tail = newTail;
        packets.size++;
        return;
    }

    struct node* prev_node = packets.head;
    while(prev_node -> next !=NULL){
        if(prev_node-> p ->hdr.seqno == cpy_recvpkt->hdr.seqno){
            // printf("Found duplicate--------! \n");
            // printf("already in %d \n", cpy_recvpkt->hdr.seqno);
            // print(&packets);
            break; 
        }
        else if(prev_node->next->p->hdr.seqno > cpy_recvpkt->hdr.seqno){
            // printf("Insert middle!!!------------------ \n"); 
            // printf("prev node: %d \n",prev_node-> p ->hdr.seqno);
            // printf("recvfrom seqno: %d \n", cpy_recvpkt->hdr.seqno);
            // printf("prev node -> next: %d \n",prev_node ->next -> p ->hdr.seqno);
            
            struct node* newNode = malloc(sizeof(struct node));
            newNode -> p = cpy_recvpkt;
            newNode -> prev = prev_node;
            newNode -> next = prev_node -> next;
            prev_node->next = newNode;
            newNode -> next -> prev = newNode;
            packets.size++;

            // print(&packets);
            break;
        }
        prev_node = prev_node -> next;
    }
        
    return;
}

void write_to_file(FILE *fp){ 

    while(!is_empty(&packets)) {
        tcp_packet *head_pkt = get_head(&packets);
        if(expec_seqno != head_pkt -> hdr.seqno) {
            break;
        }
        fseek(fp, head_pkt->hdr.seqno, SEEK_SET); 
        fwrite(head_pkt->data, 1, recvpkt->hdr.data_size, fp); //write on file
        // printf("Wirte on file: %d \n", head_pkt->hdr.seqno);
        expec_seqno = head_pkt->hdr.seqno + head_pkt->hdr.data_size; //update the expected seqno
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
            // write_to_file(fp);
            add_pkt_to_list();
            // print(&packets);
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
    printf("packets size %d \n",packets.size);
    remove_node(&packets, packets.size);
    print(&packets);

    return 0;
}
