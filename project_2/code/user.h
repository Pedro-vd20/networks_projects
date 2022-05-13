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
    int command;
    char* fname;
} port_transfer;

typedef struct
{
    struct sockaddr_in address;
    unsigned short port;
    char *data;
    int command_code;

} thread_parameters;

#endif