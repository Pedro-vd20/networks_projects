
/*
    We can have a single extra file commands.c to do all command processing
        Check if command has right num of arguments
        Check if command is valid

        Possibly could return num
            -1 means error
            0 - n are for each ith command

        Manage response codes
*/

/*
Testing:
    1. Correct authentication 
    2. Get file
        2.1 Correct port 20 stuff
        2.2 Correct multi-multi threading
        2.3 Correct merging of threads
        2.4 Correct sending of file
    3. Store file 
        (should work similar to get file)
    4. List files
        4.1 Client
        4.2 Server
    5. CWD && PWD
        5.1 Change client directory
        5.2 Change server directory
    6. Quit



*/



int main() {
    /*
        Bind stuff

        Comm to port 21

        Authenticate user

        Loop asking user

            Ask user for command

            Check command validity

            Send command

            Await response

            Display response

            IF FILE TRANSFER REQUEST

                Start thread
                
                Send server new random port

                Listen on that port for server (listen for port 20)

                Close port

                Close thread



    */

    return 0;
}