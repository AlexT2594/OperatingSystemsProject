#ifndef __METHODS_H__ // to avoud multiple inclusions of a header
#define __METHODS_H__

#include <sys/socket.h>
#include "common.h"

// Functions defined in msg_queue.c
void    initialize_queue();
void enqueue(const char *nickname, const char *to, const char* subject, const char *msg);
msg_t*  dequeue();

// Functions defined in util.c
void send_msg_by_server(int socket, char *msg);
int recv_msg(int socket, char *buf, size_t buf_len);
int send_inbox(int socket, char *nickname);
void delete_messages(int socket, char *nickname);
void exit_user(int socket);
int control_msg(char* msg);


#endif
