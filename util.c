#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "common.h"
#include "methods.h"

// Extern global variables
extern sem_t user_data_sem;
extern user_data_t* users[];
extern unsigned int current_users;

// Server sends messages adding its name
void send_msg_by_server(int socket, char *msg) {
	int ret;
    char msg_by_server[MSG_SIZE];
    sprintf(msg_by_server, "%s%c%s", SERVER_NICKNAME, MSG_DELIMITER_CHAR, msg);
    int bytes_left = strlen(msg_by_server);
    int bytes_sent = 0;
    
    while(bytes_left > 0){
    	ret = send(socket,msg_by_server + bytes_sent, bytes_left, 0);
    	if(ret == -1 && errno == EINTR) continue;
    	ERROR_HELPER(ret,"Error while server was writing on socket");

    	bytes_left -= ret;
    	bytes_sent += ret;
    }
    
}

int recv_msg(int socket, char *buf, size_t buf_len){
	int ret;
	int bytes_read = 0;

	// We don't consider messages longer than buf_len
	while(bytes_read <= buf_len){
		ret = recv(socket, buf + bytes_read , 1 ,0);

		if (ret == 0) return -1; // Client closed the socket
        if (ret == -1 && errno == EINTR) continue;
        ERROR_HELPER(ret, "Errore nella lettura da socket");

        // Controlling last byte
        if (buf[bytes_read++] == '\n') break; // End of the message

	}

    // When a message is received correctly '\n' is replaced with '\0'
    buf[--bytes_read] = '\0';
    return bytes_read; // Now bytes_read == strlen(buf)
}

int send_inbox(int socket, char *nickname) {

	char* token;

	char user_from[NICKNAME_SIZE];
    char user_subject[MSG_SIZE];
    char user_msg[MSG_SIZE];
	
	char line[LINE_SIZE];

	int ret = sem_wait(&user_data_sem);
    ERROR_HELPER(ret, "Error while calling sem_wait on user_data_sem");

	FILE *fp = fopen(nickname,"r");

	if(!fp){
		send_msg_by_server(socket,line);

		ret = sem_post(&user_data_sem);
    	ERROR_HELPER(ret, "Error while calling sem_post on user_data_sem");
		return 0;
	}

	while(fgets(line,sizeof(line),fp) != NULL){

        token = strtok(line,"|");
        sprintf(user_from,"%s",token);

        token = strtok(NULL,"|");
        sprintf(user_subject,"%s",token);

        token = strtok(NULL,"|");
        sprintf(user_msg,"%s",token);

        sprintf(line,"FROM:%s|SUBJECT:%s|MESSAGE:%s",user_from,user_subject,user_msg);

		send_msg_by_server(socket,line);
	}

	fclose(fp);

	ret = sem_post(&user_data_sem);
    ERROR_HELPER(ret, "Error while calling sem_post on user_data_sem");
	
	return 1;
}

void delete_messages(int socket, char *nickname){

	char buf[MSG_SIZE];
	char line[LINE_SIZE];
	char delete_nickname[NICKNAME_SIZE];

	char* token;

	char user_from[NICKNAME_SIZE];

	if(!send_inbox(socket,nickname)) return;
	sprintf(buf,"Please insert the name of the contact you want to delete\n");
	send_msg_by_server(socket,buf);

	recv_msg(socket,buf,MSG_SIZE);

    strcpy(delete_nickname,buf);

	int ret = sem_wait(&user_data_sem);
    ERROR_HELPER(ret, "Error while calling sem_wait on user_data_sem");

    FILE *inFile = fopen(nickname,"r");
    FILE *outFile = fopen("tmp","w+");

	if(!inFile){
		send_msg_by_server(socket,line);

		ret = sem_post(&user_data_sem);
    	ERROR_HELPER(ret, "Error while calling sem_post on user_data_sem");
		return;
	}

    while(fgets(line,sizeof(line),inFile) != NULL){

        strcpy(buf,line);

    	token = strtok(buf,"|");
        sprintf(user_from,"%s",token);

        if(strcmp(user_from,delete_nickname) != 0){
        	fprintf(outFile,"%s",line);
        }

    }

    fclose(outFile);
    fclose(inFile);
    remove(nickname);
    rename("tmp",nickname);

   	ret = sem_post(&user_data_sem);
    ERROR_HELPER(ret, "Error while calling sem_post on user_data_sem");

    send_inbox(socket,nickname);

}

void exit_user(int socket) {
    int ret;
    char buf[MSG_SIZE];

    ret = sem_wait(&user_data_sem);
    ERROR_HELPER(ret, "Error while calling sem_wait on user_data_sem");

    int i;
    for (i = 0; i < current_users; i++) {
        if (users[i]->socket == socket) {
            //notify user he has finished
            sprintf(buf,"exit\n");
            send_msg_by_server(socket,buf);
            
            free(users[i]);

            for (; i < current_users - 1; i++)
                users[i] = users[i+1]; // shift di 1 per tutti gli elementi successivi
            current_users--;

            ret = sem_post(&user_data_sem);
            ERROR_HELPER(ret, "Error while calling sem_post on user_data_sem");

            return;
        }
    }

    ret = sem_post(&user_data_sem);
    ERROR_HELPER(ret, "Error while calling sem_post on user_data_sem");
  
}

int control_msg(char* msg) {

    int len = strlen(msg);
    if(len == 0 || msg[0] == '\n') return 0;
    if(msg[0] == '#') return 1;
    int i,counter = 0;
    for(i = 0;i < len;i++){
        if(msg[i] == '|') counter++;
    }
    if(counter != 2) return 0;

    return 1;
}