#ifndef COMPLETION_QUEUE_H
#define COMPLETION_QUEUE_H

#include "connection.h"
#include <pthread.h>

#define MAX_COMPLETIONS 64

typedef struct {
    Connection *connections[MAX_COMPLETIONS];

    int front;
    int rear;
    int count;

    pthread_mutex_t mutex;
} CompletionQueue;

int completion_queue_init(CompletionQueue *queue);

int completion_enqueue(
    CompletionQueue *queue,
    Connection *conn
);

Connection *completion_dequeue(
    CompletionQueue *queue
);

#endif