#ifndef TASK_QUEUE_H
#define TASK_QUEUE_H

#include <pthread.h>
#include "connection.h"
#define MAX_SIZE 64

typedef struct 
{
    Connection *conn;
} Task;

typedef struct
{
    Task tasks[MAX_SIZE];
    int front;
    int rear;
    int count;
    pthread_cond_t condition;
    pthread_mutex_t mutex;

    
}TaskQueue;


void enqueue(TaskQueue *queue, Task task);


Task dequeue(TaskQueue *queue);

void task_queue_init(TaskQueue *queue);

void task_queue_destroy(TaskQueue *queue);

#endif

