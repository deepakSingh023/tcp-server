#include "completion_queue.h"

int completion_queue_init(CompletionQueue *queue)
{
    queue->front = 0;
    queue->rear = 0;
    queue->count = 0;

    if (pthread_mutex_init(&queue->mutex, NULL) != 0) {
        return -1;
    }

    return 0;
}


int completion_enqueue(
    CompletionQueue *queue,
    Connection *conn
)
{
    pthread_mutex_lock(&queue->mutex);

    if (queue->count == MAX_COMPLETIONS) {
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }

    queue->connections[queue->rear] = conn;

    queue->rear = (queue->rear + 1) % MAX_COMPLETIONS;

    queue->count++;

    pthread_mutex_unlock(&queue->mutex);

    return 0;
}


Connection *completion_dequeue(
    CompletionQueue *queue
)
{
    pthread_mutex_lock(&queue->mutex);

    if (queue->count == 0) {
        pthread_mutex_unlock(&queue->mutex);
        return NULL;
    }

    Connection *conn = queue->connections[queue->front];

    queue->front = (queue->front + 1) % MAX_COMPLETIONS;

    queue->count--;

    pthread_mutex_unlock(&queue->mutex);

    return conn;
}