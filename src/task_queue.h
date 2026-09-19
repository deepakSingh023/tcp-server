
# define MAX_SIZE 64

typedef struct 
{
    int number;
} Task;

typedef struct
{
    Task tasks[MAX_SIZE];
    int front;
    int rear;
}TaskQueue;


void enqueue(TaskQueue *queue, Task task);


Task dequeue(TaskQueue *queue);

