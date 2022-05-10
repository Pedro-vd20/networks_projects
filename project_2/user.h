#ifndef USER_STRUCT
#define USER_STRUCT

typedef struct
{
    int socket;
    struct sockaddr_in address;
} client;

typedef struct {
    struct sockaddr_in address;
    int port;
    int socket;
    int command;
    char* fname;
} port_transfer;

typedef struct
{
    int cntr_socket;
    struct sockaddr_in address;
    int counter;
    unsigned short port;
    char *input;
    int command_code;

} thread_parameters;

#endif