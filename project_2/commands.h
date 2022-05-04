#ifndef COMMANDS
#define COMMANDS

/**
 * @brief maps a defined command to an integer
 * 
 * @param msg entire message sent by the user
 * @param arg field to store the argument of the command (i.e PASS arg)
 * @return int 
 *      -1  command not identified
 *      -2 syntax error
 *       0  PORT
 *       1  USER
 *       2  PASS
 *       3  STOR
 *       4  RETR
 *       5  LIST
 *       6  !LIST
 *       7  CWD
 *       8  !CWD
 *       9  PWD
 *       10 !PWD
 *       11 QUIT
 */
int parse_command(char* msg, char* arg);

/**
 * @brief reads code from server in incoming message and returns code 
 * 
 * @param msg sent by server
 * @return int code in the message, -1 if error
 */
int parse_response(char* msg);

#endif