#include <stdio.h>
#include <semaphore.h>

#include "common.h"

#define MSG_BUFFER_SIZE 256

// Circular buffer to handle a FIFO 
msg_t* msg_buffer[MSG_BUFFER_SIZE];
int read_index;
int write_index;

sem_t empty_sem;
sem_t fill_sem;
sem_t write_sem;

// Initialize the queue
void initialize_queue() {
    int ret;

    read_index = 0,
    write_index = 0;

    ret = sem_init(&empty_sem, 0, MSG_BUFFER_SIZE);
    ERROR_HELPER(ret, "Error during empty_sem initialization");

    ret = sem_init(&fill_sem, 0, 0);
    ERROR_HELPER(ret, "Error during fill_sem initialization");

    ret = sem_init(&write_sem, 0, 1);
    ERROR_HELPER(ret, "Error during write_sem initialization");
}

// Generates a message that is enqueued
// Can be used by multiple threads
void enqueue(const char *nickname, const char *to, const char* subject, const char *msg) {

    int ret;
    msg_t *msg_data = (msg_t*)malloc(sizeof(msg_t));
    sprintf(msg_data->nickname, "%s", nickname);
    sprintf(msg_data->to,"%s",to);
    sprintf(msg_data->subject,"%s",subject);
    sprintf(msg_data->msg, "%s", msg);

    ret = sem_wait(&empty_sem);
    ERROR_HELPER(ret, "Error while calling sem_wait on empty_sem");

    ret = sem_wait(&write_sem);
    ERROR_HELPER(ret, "Error while calling sem_wait on write_sem");

    msg_buffer[write_index] = msg_data;
    write_index = (write_index + 1) % MSG_BUFFER_SIZE;

    ret = sem_post(&write_sem);
    ERROR_HELPER(ret, "Error while calling sem_post on write_sem");

    ret = sem_post(&fill_sem);
    ERROR_HELPER(ret, "Error while calling sem_post on fill_sem");

}

// Gets the first message from the FIFO
// Only one thread calls it
msg_t* dequeue() {

    int ret;
    msg_t *msg = NULL;

    ret = sem_wait(&fill_sem);
    ERROR_HELPER(ret, "Error while calling sem_wait on fill_sem");

    msg = msg_buffer[read_index];
    read_index = (read_index + 1) % MSG_BUFFER_SIZE;

    ret = sem_post(&empty_sem);
    ERROR_HELPER(ret, "Error while calling sem_post on empty_sem");

    return msg;
}
