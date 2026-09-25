#ifndef THREAD_POOL_H
#define THREAD_POOL_H
#include <pthread.h>
#include "task_queue.h"
#include "completion_queue.h"
#define WORKER_COUNT 4




typedef struct 
{
    pthread_t worker[WORKER_COUNT];

    TaskQueue queue;

    CompletionQueue *completion_queue;

    int event_fd;

} ThreadPool;




void thread_pool_init(ThreadPool *pool);

void *worker_function(void *arg);



#endif

