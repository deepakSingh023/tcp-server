
#include "task_queue.h"





void enqueue(TaskQueue *queue, Task task){

    if(sizeof(queue)==0){
        queue->tasks[0] = task;
        queue->front = task.number;
        queue->rear = task.number;
        return 0;
    }

    queue->tasks[sizeof(queue)] = task;
    queue->front = task.number;
    queue->rear++;

}


Task dequeue(TaskQueue *queue){

    int back = queue->rear;

    return queue->tasks[back];
}