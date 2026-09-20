#include "thread_pool.h"
#include <stdio.h>
#include "connection.h">



void thread_pool_init(ThreadPool *pool){

    task_queue_init(&pool->queue);

    for(int i = 0; i < WORKER_COUNT ; i++){
        pthread_create(
            &pool->worker[i],
            NULL,
            worker_function,
            pool
        );
    }
}



void *worker_function(void *arg){

    ThreadPool *pool = arg;


    while (1) {
        Task task = dequeue(&pool->queue);
        Connection *conn = task.conn;

        
        printf("Worker processing connection fd=%d\n", conn->fd);
    }

    return NULL;

}