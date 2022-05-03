#ifndef USER
#define USER

typedef struct {
    int socket;
    struct sockaddr_in address;
} client;

#endif