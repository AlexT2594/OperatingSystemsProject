#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>

#define GENERIC_ERROR_HELPER(cond, errCode, msg) do {               \
        if (cond) {                                                 \
            fprintf(stderr, "%s: %s\n", msg, strerror(errCode));    \
            exit(EXIT_FAILURE);                                     \
        }                                                           \
    } while(0)

#define ERROR_HELPER(ret, msg)          GENERIC_ERROR_HELPER((ret < 0), errno, msg)
#define PTHREAD_ERROR_HELPER(ret, msg)  GENERIC_ERROR_HELPER((ret != 0), ret, msg)

// Configuration params for messages
#define MSG_SIZE            1024
#define LINE_SIZE           1024
#define NICKNAME_SIZE       128
#define PASSWORD_SIZE        128

// Messages
typedef struct msg_s {
    char    nickname[NICKNAME_SIZE];
    char    to[NICKNAME_SIZE];
    char    subject[MSG_SIZE];
    char    msg[MSG_SIZE];
} msg_t;

typedef struct handler_args_s {
    int socket_desc;
    struct sockaddr_in* client_addr;
} handler_args_t;

// Users
typedef struct user_data_s {
    int     socket;
    char    nickname[NICKNAME_SIZE];
    char    address[INET_ADDRSTRLEN];
    uint16_t    port;
} user_data_t;

// Configuration parameters 
#define DEBUG           1   // display debug messages
#define MAX_CONN_QUEUE  100  // max number of connections the server can queue
#define SERVER_ADDRESS  "127.0.0.1"
#define SERVER_COMMAND  "QUIT"
#define SERVER_PORT     2015
#define SERVER_NICKNAME "!nstant"
#define YES_COMMAND "y"
#define MAX_USERS           128

#define BUFFER_SIZE     1024
#define CLOSE_COMMAND   "#exit"
#define CLOSE_COMMAND_RECV "exit\n"
#define MSG_DELIMITER_CHAR  '|'
#define COMMAND_CHAR        '#'

#define MSG_SIZE            1024
#define LINE_SIZE           1024
#define NICKNAME_SIZE       128
#define PASSWORD_SIZE        128

// every command starts with #
#define INBOX_COMMAND        "inbox"
#define DELETE_COMMAND       "delete"
#define EXIT_COMMAND         "exit"

#endif
