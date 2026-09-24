#include "thread_pool.h"
#include <stdio.h>
#include "connection.h"
#include "router.h"
#include "http_request.h"
#include "completion_queue.h"

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




void *worker_function(void *arg)
{
    ThreadPool *pool = arg;

    while (1) {

        Task task = dequeue(&pool->queue);

        Connection *conn = task.conn;

        HttpRequest httprequest;

        int result = http_request_parse(
            &httprequest,
            conn->input.data,
            conn->input.length
        );

        if (result == -1) {
            printf("HTTP request parsing failed\n");
            continue;
        }

        int route_result = router_handle(
            &httprequest,
            conn
        );

        if (route_result == 404) {
            printf("Route not found\n");

            // create the 404 response later.
        }

        http_request_free(&httprequest);

        completion_enqueue(
            pool->completion_queue,
            conn
        );

        printf(
            "Worker processed connection fd=%d\n",
            conn->fd
        );
    }

    return NULL;
}