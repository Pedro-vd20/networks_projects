#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "commands.h"

int parse_command(char *msg, char *arg) {

    // collect first word of msg
    char *token = strtok(msg, " ");
    if (token == NULL)
    {
        return -1;
    }

    char command[50];
    strcpy(command, token);

    // process commands that don't need argument
    if (!strcmp(command, "LIST"))
    {
        return 5;
    }
    else if (!strcmp(command, "!LIST"))
    {
        return 6;
    }
    else if (!strcmp(command, "PWD"))
    {
        return 9;
    }
    else if (!strcmp(command, "!PWD"))
    {
        return 10;
    }
    else if (!strcmp(command, "QUIT"))
    {
        return 11;
    }

    int ret = -1;

    // process commands with argument
    if (!strcmp(command, "PORT"))
    {
        ret = 0;
    }
    else if (!strcmp(command, "USER"))
    {
        ret = 1;
    }
    else if (!strcmp(command, "PASS"))
    {
        ret = 2;
    }
    else if (!strcmp(command, "STOR"))
    {
        ret = 3;
    }
    else if (!strcmp(command, "RETR"))
    {
        ret = 4;
    }
    else if (!strcmp(command, "CWD"))
    {
        ret = 7;
    }
    else if (!strcmp(command, "!CWD"))
    {
        ret = 8;
    }

    if(ret != -1) {
        // collect argument
        token = strtok(NULL, " ");

        if (token == NULL)
        {
            return -2;
        }
        token = strtok(token, "\n");

        strcpy(arg, token);
    }
    
    return ret;
}

int parse_response(char *msg) {
    // collect first word
    char *token = strtok(msg, " ");
    if (token == NULL)
    {
        return -1;
    }

    // check if token is a number
    for (int i = 0; i < strlen(token); ++i)
    {
        if (token[i] < '0' || token[i] > '9')
        {
            return -1;
        }
    }

    return atoi(token);
}