CC = gcc -Wall -O0 -g

all:  client server

client: client.c
	$(CC) -o client client.c -lpthread

server: server.c msg_queue.c util.c
	$(CC) -o server server.c msg_queue.c util.c -lpthread

:phony
clean:
	rm -f client server
