#ifndef USER_STRUCT
#define USER_STRUCT

typedef struct {
    int socket;
    struct sockaddr_in address;
} client;

#endif