#ifndef USER_STRUCT
#define USER_STRUCT

typedef struct
{
    int socket;
    struct sockaddr_in address;
} client;

typedef struct
{
    int cntr_socket;
    struct sockaddr_in address;
    int counter;
    unsigned short port;
    char *input;

} thread_parameters;

#endif