#ifndef DATA_TRANSFER
#define DATA_TRANSFER

int receive_file(int sockfd, char *filename);
int send_file(int sockfd, char *filename);
#endif