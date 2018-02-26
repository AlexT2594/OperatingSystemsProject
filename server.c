#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>  // htons()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <semaphore.h>
#include <string.h>
#include <signal.h>

#include "common.h"
#include "methods.h"


// Data to handle connected users
user_data_t* users[MAX_USERS];
unsigned int current_users;
sem_t user_data_sem;

void broadcast(msg_t* msg) {

	int user_to_online = 0;
    int ret;
    ret = sem_wait(&user_data_sem);
    ERROR_HELPER(ret, "Error while calling sem_wait on user_data_sem");

    int i;
    char msg_to_send[MSG_SIZE];
    sprintf(msg_to_send, "FROM:%s%cSubject:%s%cMessage:%s", msg->nickname, MSG_DELIMITER_CHAR,msg->subject,MSG_DELIMITER_CHAR, msg->msg);

    for (i = 0; i < current_users; i++) {
        if (strcmp(msg->nickname, users[i]->nickname) != 0 && strcmp(msg->to,users[i]->nickname) == 0) {
        	user_to_online = 1;
            while ( (ret = send(users[i]->socket, msg_to_send, strlen(msg_to_send), 0)) < 0 ) {
        		if (errno == EINTR) continue;
        		ERROR_HELPER(-1, "Cannot write to the socket");
    		}
        }
    }

    if(!user_to_online){
    	enqueue(msg->nickname,msg->to,msg->subject,msg->msg);
    }

    ret = sem_post(&user_data_sem);
    ERROR_HELPER(ret, "Error while calling sem_post on user_data_sem");
}

void* broadcast_routine(void *args) {
    while (1) {
        msg_t *msg = dequeue();
        broadcast(msg);
        free(msg);
    }
}

void authentication(int socket_desc,struct sockaddr_in* address,char* user_nickname,handler_args_t* args){
	int ret;

	int authenticated = 0;
	char line[MSG_SIZE];
    char nickname[NICKNAME_SIZE];
    char *token;
    
    char buf[MSG_SIZE];

    sprintf(buf,"Please insert your name followed by your password with a space in between\n");

    send_msg_by_server(socket_desc,buf);

    ret = sem_wait(&user_data_sem);
    ERROR_HELPER(ret, "Error while calling sem_wait on user_data_sem");


    FILE *fp;
    fp = fopen("users.db","r");

    // authentication loop
    while (!authenticated) {
    	rewind(fp);
        // read message from client
        if(recv_msg(socket_desc,line,MSG_SIZE) == -1){
            fclose(fp);
            ret = sem_post(&user_data_sem);
            ERROR_HELPER(ret, "Error while calling sem_post on user_data_sem");

            // close socket
            ret = close(socket_desc);
            ERROR_HELPER(ret, "Cannot close socket for incoming connection");

            if (DEBUG) fprintf(stderr, "Thread created to handle the request has completed.\n");

            free(args->client_addr); // do not forget to free this buffer!
            free(args);
            pthread_exit(NULL);  
        }

        sprintf(buf,"%s\n",line);

        // check line
        while(fgets(line,MSG_SIZE,fp) != NULL) {

        	if(strcmp(line,buf) == 0){
        		authenticated = 1;
        		// Create new user
			    user_data_t* new_user = (user_data_t*)malloc(sizeof(user_data_t));
			    new_user->socket = socket_desc;

     		    token = strtok(buf," ");
        		sprintf(nickname,"%s",token);
			    
			    sprintf(new_user->nickname, "%s", nickname);
			    sprintf(user_nickname,"%s",nickname);
			    inet_ntop(AF_INET, &(address->sin_addr), new_user->address, INET_ADDRSTRLEN);
			    new_user->port = ntohs(address->sin_port);
			    users[current_users++] = new_user;	
        	}
        }

		if(authenticated){

			sprintf(buf,"Welcome %s to the most simple messagging app in the world!!To send a message to your friend just tipe FRIEND_NAME|SUBJECT|MESSAGE and we'll do the magic for you!! You can see your messages by pressing #inbox, delete certain messages with #delete and exit from our app with #exit. Enjoy your time and be prepared to receive messages in an !nstant.\n",user_nickname);

		}else{
			sprintf(buf,"Wrong name and password combination\n");
		}        

        send_msg_by_server(socket_desc,buf);
    }
    fclose(fp);

    ret = sem_post(&user_data_sem);
    ERROR_HELPER(ret, "Error while calling sem_post on user_data_sem");

}

void registration(int socket_desc,handler_args_t* args){
    int ret;

    char buf[MSG_SIZE];
    char nickname[NICKNAME_SIZE];
    char line[NICKNAME_SIZE];
    char password[MSG_SIZE];
    char *token;
    int  registered = 0;

    sprintf(buf,"Please insert the desired username\n");

    send_msg_by_server(socket_desc,buf);


    ret = sem_wait(&user_data_sem);
    ERROR_HELPER(ret, "Error while calling sem_wait on user_data_sem");


    FILE *fp;
    fp = fopen("users.db","r");

    // Is user available?

    // registration loop
    while (!registered) {
    	registered = 1;
    	rewind(fp);
        // read message from client
        if(recv_msg(socket_desc,nickname,MSG_SIZE) == -1){
            fclose(fp);
            ret = sem_post(&user_data_sem);
            ERROR_HELPER(ret, "Error while calling sem_post on user_data_sem");

            // close socket
            ret = close(socket_desc);
            ERROR_HELPER(ret, "Cannot close socket for incoming connection");

            if (DEBUG) fprintf(stderr, "Thread created to handle the request has completed.\n");

            free(args->client_addr); // do not forget to free this buffer!
            free(args);
            pthread_exit(NULL);  
        }

        // check nicknames
        while(fgets(line,MSG_SIZE,fp) != NULL) {
        	token = strtok(line," ");
        	sprintf(buf,"%s",token);

        	if(strcmp(nickname,buf) == 0){
        		registered = 0;		
        	}
        }

		if(registered){
			// Ask for password
			sprintf(buf,"Username available! %s please insert your password\n",nickname);
			send_msg_by_server(socket_desc,buf);

            // read message from client
            if(recv_msg(socket_desc,password,MSG_SIZE) == -1){
                fclose(fp);

                ret = sem_post(&user_data_sem);
                ERROR_HELPER(ret, "Error while calling sem_post on user_data_sem");

                // close socket
                ret = close(socket_desc);
                ERROR_HELPER(ret, "Cannot close socket for incoming connection");

                if (DEBUG) fprintf(stderr, "Thread created to handle the request has completed.\n");

                free(args->client_addr); // do not forget to free this buffer!
                free(args);
                pthread_exit(NULL);  
            }

			fclose(fp);

			fp = fopen("users.db","a");
			fprintf(fp,"%s %s\n",nickname,password);

			sprintf(buf,"We have finished your registration\n");

		}else{
			sprintf(buf,"Wrong username\n");
		}        

        send_msg_by_server(socket_desc,buf);
    }
    fclose(fp);

    ret = sem_post(&user_data_sem);
    ERROR_HELPER(ret, "Error while calling sem_post on user_data_sem");

}

void* connection_handler(void* arg) {
    handler_args_t* args = (handler_args_t*)arg;

    int socket_desc = args->socket_desc;
    struct sockaddr_in* client_addr = args->client_addr;

    int ret;

    char user_nickname[NICKNAME_SIZE];
    char user_to[NICKNAME_SIZE];
    char user_subject[MSG_SIZE];
    char user_msg[MSG_SIZE];

    char copy[MSG_SIZE];

    char buf[MSG_SIZE];

    char* yes_command = YES_COMMAND;
    size_t yes_command_len = strlen(yes_command);

    // parse client IP address and port
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr->sin_addr), client_ip, INET_ADDRSTRLEN);
    //uint16_t client_port = ntohs(client_addr->sin_port); // port number is an unsigned short

    // send welcome message
    sprintf(buf, "Hi! Messagging app here.Are you already registered?[y/n]\n");

    send_msg_by_server(socket_desc,buf);
	
    //read available data one byte at a time till '\n' is found
    recv_msg(socket_desc,buf,MSG_SIZE);

    if(!memcmp(buf,yes_command,yes_command_len)){
       authentication(socket_desc,client_addr,user_nickname,args); 
    }else{
        registration(socket_desc,args);
        authentication(socket_desc,client_addr,user_nickname,args);
    } 
 	
    // main loop
    while (1) {
        // read message from client
        if(recv_msg(socket_desc,buf,MSG_SIZE) == -1){
            //client has closed socket
            exit_user(socket_desc);
            break;
        }

        if(!control_msg(buf)) continue;

        if(buf[0] == COMMAND_CHAR){
        	if(strcmp(buf + 1,INBOX_COMMAND) == 0){
                fprintf(stderr,"Inbox command requested\n");
        		send_inbox(socket_desc,user_nickname);
        		continue;
        	}else if(strcmp(buf + 1,DELETE_COMMAND) == 0){
        		delete_messages(socket_desc,user_nickname);
        		continue;
        	}else if(strcmp(buf + 1,EXIT_COMMAND) == 0){
                exit_user(socket_desc);
                break;
            }else{
                continue;
            }
        }
     	
        strcpy(copy,buf);

        char* token;
        token = strtok(copy,"|");
        sprintf(user_to,"%s",token);

        token = strtok(NULL,"|");
        sprintf(user_subject,"%s",token);

        token = strtok(NULL,"|");
        sprintf(user_msg,"%s\n",token);

        enqueue(user_nickname,user_to,user_subject,user_msg);

        ret = sem_wait(&user_data_sem);
        ERROR_HELPER(ret, "Error while calling sem_wait on user_data_sem");

        // Save message to local_user_db
    	FILE *fp = fopen(user_to,"a");
    	fprintf(fp,"%s|%s|%s",user_nickname,user_subject,user_msg);
    	fclose(fp);

        ret = sem_post(&user_data_sem);
        ERROR_HELPER(ret, "Error while calling sem_post on user_data_sem");


    }

    // close socket
    ret = close(socket_desc);
    ERROR_HELPER(ret, "Cannot close socket for incoming connection");

    if (DEBUG) fprintf(stderr, "Thread created to handle the request has completed.\n");

    free(args->client_addr); // do not forget to free this buffer!
    free(args);
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

    int client_desc;

    // Initialize data for users
    current_users = 0;
    ret = sem_init(&user_data_sem, 0, 1);
    ERROR_HELPER(ret, "Error while initializing user_data_sem");

    // Initialize queue for messages
    initialize_queue();

    // Launches thread for broadcast
    pthread_t broadcast_thread;
    ret = pthread_create(&broadcast_thread, NULL, broadcast_routine, NULL);
    PTHREAD_ERROR_HELPER(ret, "Errore nella creazione di un thread");

    ret = pthread_detach(broadcast_thread);
    PTHREAD_ERROR_HELPER(ret, "Errore nel detach di un thread");

    // Some fields are required to be filled with 0
    struct sockaddr_in server_addr = {0};

    int sockaddr_len = sizeof(struct sockaddr_in); // We will reuse it for accept()

    // Initialize socket for listening
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    ERROR_HELPER(socket_desc, "Could not create socket");

    server_addr.sin_addr.s_addr = INADDR_ANY; // we want to accept connections from any interface
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(SERVER_PORT); // don't forget about network byte order!

    // We enable SO_REUSEADDR to quickly restart our server after a crash:
    int reuseaddr_opt = 1;
    ret = setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_opt, sizeof(reuseaddr_opt));
    ERROR_HELPER(ret, "Cannot set SO_REUSEADDR option");

    // Bind address to socket
    ret = bind(socket_desc, (struct sockaddr*) &server_addr, sockaddr_len);
    ERROR_HELPER(ret, "Cannot bind address to socket");

    // Start listening
    ret = listen(socket_desc, MAX_CONN_QUEUE);
    ERROR_HELPER(ret, "Cannot listen on socket");

    // We allocate client_addr dynamically and initialize it to zero
    struct sockaddr_in* client_addr = calloc(1, sizeof(struct sockaddr_in));


    // Loop to manage incoming connections spawning handler threads
    while (1) {
        // Accept incoming connection
        client_desc = accept(socket_desc, (struct sockaddr*) client_addr, (socklen_t*) &sockaddr_len);
        if (client_desc == -1 && errno == EINTR) continue; // check for interruption by signals
        ERROR_HELPER(client_desc, "Cannot open socket for incoming connection");

        if (DEBUG) fprintf(stderr, "Incoming connection accepted...\n");

        pthread_t thread;

        // Put arguments for the new thread into a buffer
        handler_args_t* thread_args = malloc(sizeof(handler_args_t));
        thread_args->socket_desc = client_desc;
        thread_args->client_addr = client_addr;

        if (pthread_create(&thread, NULL, connection_handler, (void*)thread_args) != 0) {
            fprintf(stderr, "Can't create a new thread, error %d\n", errno);
            exit(EXIT_FAILURE);
        }

        if (DEBUG) fprintf(stderr, "New thread created to handle the request!\n");

        pthread_detach(thread); // I won't phtread_join() on this thread

        // We can't just reset fields: we need a new buffer for client_addr!
        client_addr = calloc(1, sizeof(struct sockaddr_in));
    }

    exit(EXIT_SUCCESS); // This will never be executed
}