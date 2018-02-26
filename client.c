#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>  // htons() and inet_addr()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h>
#include <signal.h>

#include "common.h"

int shouldStop = 0;

void* receiveMessage(void* arg) {
    int socket_desc = (int)(long)arg;

    /* select() uses sets of descriptors and a timeval interval. The
     * method returns when either an event occurs on a descriptor in
     * the sets during the given interval, or when that time elapses.
     *
     * The first argument for select is the maximum descriptor among
     * those in the sets plus one. Note also that both the sets and
     * the timeval argument are modified by the call, so you should
     * reinitialize them across multiple invocations.
     *
     * On success, select() returns the number of descriptors in the
     * given sets for which data may be available, or 0 if the timeout
     * expires before any event occurs. */
    struct timeval timeout;
    fd_set read_descriptors;
    int nfds = socket_desc + 1;

    char buf[BUFFER_SIZE];


    while (!shouldStop) {
        int ret;

        // Check every 1.5 seconds 
        timeout.tv_sec  = 1;
        timeout.tv_usec = 500000;

        FD_ZERO(&read_descriptors);
        FD_SET(socket_desc, &read_descriptors);

        ret = select(nfds, &read_descriptors, NULL, NULL, &timeout);

        if (ret == -1 && errno == EINTR) continue;
        ERROR_HELPER(ret, "Unable to select()");

        if (ret == 0) continue; // Timeout expired
        
        // At this point (ret==1) our message has been received!
        
        // Read available data one byte at a time till '\n' is found
        int bytes_read = 0;
        
        while (1) {
            ret = recv(socket_desc, buf + bytes_read, 1, 0);
            
            if (ret == -1 && errno == EINTR) continue;
            ERROR_HELPER(ret, "Cannot read from socket");
            
            if (ret == 0) {
                fprintf(stderr, "[WARNING] Endpoint closed the connection unexpectedly. Exiting...\n");
                shouldStop = 1;
                pthread_exit(NULL);
            }
            
            // We use post-increment on bytes_read so we first read the
            // byte that has just been written and then we increment
            if (buf[bytes_read++] == '\n') break;
        }

            buf[bytes_read] = '\0';

            char *token;
            char copy[MSG_SIZE];
            char msg_buffer[MSG_SIZE];

            strcpy(copy,buf);

            token = strtok(copy,"|");

            token = strtok(NULL,"|");
            sprintf(msg_buffer,"%s",token);

            if(strcmp(msg_buffer,CLOSE_COMMAND_RECV) == 0) {
                break;
            }
        
            printf("==> %s", buf);
    }

    pthread_exit(NULL);
}

void* sendMessage(void* arg) {
    int socket_desc = (int)(long)arg;

    char* close_command = CLOSE_COMMAND;
    size_t close_command_len = strlen(close_command);

    char buf[BUFFER_SIZE];

    while (!shouldStop) {
        /* Read a line from stdin: fgets() reads up to sizeof(buf)-1
         * bytes and on success returns the first argument passed.
         * Note that '\n' is added at the end of the message when ENTER
         * is pressed: we can thus use it as our message delimiter! */
        if (fgets(buf, sizeof(buf), stdin) != (char*)buf) {
            fprintf(stderr, "Error while reading from stdin, exiting...\n");
            exit(EXIT_FAILURE);
        }

        // Check if the endpoint has closed the connection
        if (shouldStop) break;

        // Compute number of bytes to send (skip string terminator '\0')
        size_t msg_len = strlen(buf);
        
        int ret, bytes_sent = 0;

        // Make sure that all bytes are sent!
        while (bytes_sent < msg_len) {
            ret = send(socket_desc, buf + bytes_sent, msg_len - bytes_sent, MSG_NOSIGNAL);
            if (ret == -1 && errno == EINTR) continue;
            ERROR_HELPER(ret, "Cannot write to socket");
            bytes_sent += ret;
        }

        // if we just sent a #quit command, we need to update shouldStop!
        // (note that we subtract 1 to skip the message delimiter '\n')
        if (msg_len - 1 == close_command_len && !memcmp(buf, close_command, close_command_len)) {
            shouldStop = 1;
            fprintf(stderr, "Chat session terminated.\n");
        }
    }

    pthread_exit(NULL);
}

int socket_desc;

void signal_handler(int signal) {

    // close socket
    int ret = close(socket_desc);
    ERROR_HELPER(ret, "Cannot close socket");

    exit(EXIT_SUCCESS);
}

int main(int argc, char* argv[]) {
    int ret;

    sigset_t set;
    struct sigaction act;

    sigemptyset(&set);
    sigaddset(&set,SIGPIPE);
    sigaddset(&set,SIGINT);
    sigaddset(&set,SIGTERM);
    act.sa_sigaction = (void*)signal_handler; 
    act.sa_mask =  set;
    act.sa_flags = SA_SIGINFO;
    act.sa_restorer = NULL;
    sigaction(SIGPIPE,&act,NULL);
    sigaction(SIGINT,&act,NULL);
    sigaction(SIGTERM,&act,NULL);

    // variables for handling a socket
    struct sockaddr_in server_addr = {0}; // some fields are required to be filled with 0

    // create a socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    ERROR_HELPER(socket_desc, "Could not create socket");

    // set up parameters for the connection
    server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(SERVER_PORT); // don't forget about network byte order!

    // initiate a connection on the socket
    ret = connect(socket_desc, (struct sockaddr*) &server_addr, sizeof(struct sockaddr_in));
    ERROR_HELPER(ret, "Could not create connection");

    //if (DEBUG) fprintf(stderr, "Connection established!\n");

    char buf[1024];

    //read the welcoming message
    //read available data one byte at a time till '\n' is found
    printf("\
  _           _              _   \n\
 | |         | |            | |  \n\
 | |_ __  ___| |_ __ _ _ __ | |_ \n\
 | | '_ \\/ __| __/ _` | '_ \\| __|\n\
 |_| | | \\__ \\ || (_| | | | | |_ \n\
 (_)_| |_|___/\\__\\__,_|_| |_|\\__|\n\n");

    int bytes_read = 0;
    
    while (1) {
        ret = recv(socket_desc, buf + bytes_read, 1, 0);
        
        if (ret == -1 && errno == EINTR) continue;
        ERROR_HELPER(ret, "Cannot read from socket");
        
        if (ret == 0) {
            fprintf(stderr, "[WARNING] Endpoint closed the connection unexpectedly. Exiting...\n");
            break;
        }
        
        // we use post-increment on bytes_read so we first read the
        // byte that has just been written and then we increment
        if (buf[bytes_read++] == '\n') break;
    }

    //insert \0 instead of \n
    buf[--bytes_read] = '\0';
    printf("==> %s\n", buf); 



    pthread_t client_threads[2];

    ret = pthread_create(&client_threads[0], NULL, receiveMessage, (void*)(long)socket_desc);
    PTHREAD_ERROR_HELPER(ret, "Cannot create thread for receiving messages");

    ret = pthread_create(&client_threads[1], NULL, sendMessage, (void*)(long)socket_desc);
    PTHREAD_ERROR_HELPER(ret, "Cannot create thread for sending messages");

    // wait for termination
    ret = pthread_join(client_threads[0], NULL);
    PTHREAD_ERROR_HELPER(ret, "Cannot join on thread for receiving messages");

    ret = pthread_join(client_threads[1], NULL);
    PTHREAD_ERROR_HELPER(ret, "Cannot join on thread for sending messages");

    // close socket
    ret = close(socket_desc);
    ERROR_HELPER(ret, "Cannot close socket");

    exit(EXIT_SUCCESS);
}
