#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <assert.h>

#include"packet.h"
#include"common.h"
#include "linked_list.h"

#define STDIN_FD    0
#define RETRY 120 //millisecond

int next_seqno=0;
int send_base=0;
int window_size = 10;

int sockfd, serverlen;
struct sockaddr_in serveraddr;
struct itimerval timer; 
tcp_packet *sndpkt;
tcp_packet *recvpkt;
sigset_t sigmask;       

linked_list sliding_window;

void resend_packets(int sig)
{
    if (sig == SIGALRM)
    {
        //Resend all packets range between 
        //sendBase and nextSeqNum
        VLOG(INFO, "Timout happend");
        if(!is_empty(&sliding_window)) {
            struct node* head = sliding_window.head;
            while(head != NULL) {
                if(sendto(sockfd, head->p, TCP_HDR_SIZE + get_data_size(sndpkt), 0, 
                            ( const struct sockaddr *)&serveraddr, serverlen) < 0)
                {
                    error("sendto");
                }
                head = head->next;
            }
        }
    }
}


void start_timer()
{
    sigprocmask(SIG_UNBLOCK, &sigmask, NULL);
    setitimer(ITIMER_REAL, &timer, NULL);
}


void stop_timer()
{
    sigprocmask(SIG_BLOCK, &sigmask, NULL);
}


/*
 * init_timer: Initialize timer
 * delay: delay in milliseconds
 * sig_handler: signal handler function for re-sending unACKed packets
 */
void init_timer(int delay, void (*sig_handler)(int)) 
{
    signal(SIGALRM, resend_packets);
    timer.it_interval.tv_sec = delay / 1000;    // sets an interval of the timer
    timer.it_interval.tv_usec = (delay % 1000) * 1000;  
    timer.it_value.tv_sec = delay / 1000;       // sets an initial value
    timer.it_value.tv_usec = (delay % 1000) * 1000;

    sigemptyset(&sigmask);
    sigaddset(&sigmask, SIGALRM);
}


int main (int argc, char **argv)
{
    int eof_flag = 0;
    int portno, len;
    int next_seqno;
    char *hostname;
    char buffer[DATA_SIZE];
    FILE *fp;

    /* check command line arguments */
    if (argc != 4) {
        fprintf(stderr,"usage: %s <hostname> <port> <FILE>\n", argv[0]);
        exit(0);
    }
    hostname = argv[1];
    portno = atoi(argv[2]);
    fp = fopen(argv[3], "r");
    if (fp == NULL) {
        error(argv[3]);
    }

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");


    /* initialize server server details */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serverlen = sizeof(serveraddr);

    /* covert host into network byte order */
    if (inet_aton(hostname, &serveraddr.sin_addr) == 0) {
        fprintf(stderr,"ERROR, invalid host %s\n", hostname);
        exit(0);
    }

    /* build the server's Internet address */
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(portno);

    assert(MSS_SIZE - TCP_HDR_SIZE > 0);

    //Stop and wait protocol

    init_timer(RETRY, resend_packets);
    next_seqno = 0;
    sliding_window.size = 0;
    int expected_ack = 0;

    while (1)
    {
        // Send packets
        // only if current window isn't 10 and the end of file hasn't been reached
        while((!eof_flag) && (sliding_window.size < window_size)) {
            len = fread(buffer, 1, DATA_SIZE, fp);
            // If lenght is 0, stop adding packets to sliding window
            // Set end of file as true
            if(len <= 0) {
                // Set end of file flag and break
                eof_flag = 1;
                break;
            }

            // make packet
            send_base = next_seqno;
            next_seqno = send_base + len;
            sndpkt = make_packet(len);
            memcpy(sndpkt->data, buffer, len);
            sndpkt->hdr.seqno = send_base;
            
            // add packet to sliding window
            add_node(&sliding_window, sndpkt);

            VLOG(DEBUG, "Sending packet %d to %s", 
                    send_base, inet_ntoa(serveraddr.sin_addr));
            /*
             * If the sendto is called for the first time, the system will
             * will assign a random port number so that server can send its
             * response to the src port.
             */
            // send packet
            if(sendto(sockfd, sndpkt, TCP_HDR_SIZE + get_data_size(sndpkt), 0, 
                        ( const struct sockaddr *)&serveraddr, serverlen) < 0)
            {
                error("sendto");
            }
        } 

        // EOF reached
        if(eof_flag && is_empty(&sliding_window)) {
            // send eof packet
            VLOG(INFO, "End Of File has been reached");
            sndpkt = make_packet(0);
            sendto(sockfd, sndpkt, TCP_HDR_SIZE,  0,
                    (const struct sockaddr *)&serveraddr, serverlen);
            add_node(&sliding_window, sndpkt);
            break;
        }
        
        sndpkt = get_head(&sliding_window);
        
        expected_ack = sndpkt->hdr.seqno + sndpkt->hdr.data_size;
                
        //Wait for ACK
        do {
            start_timer();
            //ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
            //struct sockaddr *src_addr, socklen_t *addrlen);

            // check for incoming packets
            do
            {
                if(recvfrom(sockfd, buffer, MSS_SIZE, 0,
                            (struct sockaddr *) &serveraddr, (socklen_t *)&serverlen) < 0)
                {
                    error("recvfrom");
                }

                recvpkt = (tcp_packet *)buffer;
                printf("%d \n", get_data_size(recvpkt));
                assert(get_data_size(recvpkt) <= DATA_SIZE);

            

            } while(recvpkt->hdr.ackno < expected_ack);    //ignore duplicate ACKs

            stop_timer();
            /*resend pack if don't recv ACK */

        } while(recvpkt->hdr.ackno < expected_ack);      

        sndpkt = get_head(&sliding_window);
        // while sndpckt seq # less than ack, remove packet
        while(sndpkt->hdr.seqno < recvpkt->hdr.ackno) {
            remove_node(&sliding_window, 1);
            if(is_empty(&sliding_window)) {
                break;
            }
            sndpkt = get_head(&sliding_window);
        }

    }

    delete_list(&sliding_window);
    return 0;

}



// Notes:
/* 

Am I getting stuck in the loop to remove nodes?
Check the "re-stocking" of packets for the sliding window

*/