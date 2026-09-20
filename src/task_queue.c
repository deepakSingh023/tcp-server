
#include "task_queue.h"


#include <pthread.h>



void enqueue(TaskQueue *queue, Task task){

    pthread_mutex_lock(&queue->mutex);

    if(queue->count == MAX_SIZE){
        pthread_mutex_unlock(&queue->mutex);
        return;
    }

    queue->tasks[queue->rear] = task;

    queue->count++;
    queue->rear = (queue->rear + 1) % MAX_SIZE;

    pthread_cond_signal(&queue->condition);

    pthread_mutex_unlock(&queue->mutex);

}



Task dequeue(TaskQueue *queue)
{
    pthread_mutex_lock(&queue->mutex);

    while (queue->count == 0) {
        pthread_cond_wait(
            &queue->condition,
            &queue->mutex
        );
    }

    Task task = queue->tasks[queue->front];

    queue->front = (queue->front + 1) % MAX_SIZE;
    queue->count--;

    pthread_mutex_unlock(&queue->mutex);

    return task;
}

void task_queue_init(TaskQueue *queue){
    queue->front = 0;
    queue->rear = 0;
    queue->count=0;

    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->condition, NULL);
}

void task_queue_destroy(TaskQueue *queue)
{
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->condition);
}