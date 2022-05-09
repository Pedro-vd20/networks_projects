#ifndef CONSTANTS
#define CONSTANTS

// threads
#define NUM_THREADS 32
#define NUM_F_TRANSFERS 5

// ports
#define CTR_PORT 2021 // 21 blocked on linux
#define TRANSFER_PORT 2020

// commands
#define USER 1
#define PASS 2
#define PORT 0
#define STOR 3
#define RETR 4
#define LIST 5
#define L_LIST 6 // local list, !LIST
#define CWD 7
#define L_CWD 8
#define PWD 9
#define L_PWD 10
#define QUIT 11

// responses
#define NOT_LOGGED_IN "530 Not logged in\n"
#define LEN_NOT_LOGGED_IN 18
#define USERNAME_OK "331 Username OK, need password\n"
#define LEN_USERNAME_OK 31
#define ERROR "501 Syntax error in parameters or arguments\n"
#define LEN_ERROR 44
#define NOT_IMPLEMENTED "202 Command not implemented\n"
#define LEN_NOT_IMPLEMENTED 28 
#define AUTHENTICATED "230 User logged in, proceed\n"
#define LEN_AUTHENTICATED 28 

#endif