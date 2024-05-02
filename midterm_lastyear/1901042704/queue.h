
#pragma

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>


typedef struct
{
    pid_t *data; 
    int front;
    int rear;
    int size;
    int capacity;
} Queue;

// Initialize an empty queue
void initializeQueue(Queue *queue)
{
    queue->data = NULL; 
    queue->front = 0;
    queue->rear = 0;
    queue->size = 0;
    queue->capacity = 0;
}

// Check if the queue is empty
int isQueueEmpty(Queue *queue)
{
    return queue->size == 0;
}

// Enqueue a client into the queue
void enqueue(Queue *queue, pid_t client)
{
    if (queue->size == queue->capacity)
    {
        // Increase the capacity by doubling its size
        int newCapacity = (queue->capacity == 0) ? 1 : queue->capacity * 2;
        pid_t *newData = realloc(queue->data, newCapacity * sizeof(pid_t));
        if (newData == NULL)
        {
            fprintf(stderr, "Failed to allocate memory for queue\n");
            exit(EXIT_FAILURE);
        }
        queue->data = newData;
        queue->capacity = newCapacity;
    }

    queue->data[queue->rear] = client;
    queue->rear = (queue->rear + 1) % queue->capacity;
    queue->size++;
}

// Dequeue a client from the queue
pid_t dequeue(Queue *queue)
{
    if (isQueueEmpty(queue))
    {
        fprintf(stderr, "Cannot dequeue from an empty queue\n");
        exit(EXIT_FAILURE);
    }

    pid_t client = queue->data[queue->front];
    queue->front = (queue->front + 1) % queue->capacity;
    queue->size--;
    return client;
}

// Peek at the first element in the queue
pid_t peek(Queue *queue)
{
    if (isQueueEmpty(queue))
    {
        fprintf(stderr, "Cannot peek an empty queue\n");
        exit(EXIT_FAILURE);
    }

    return queue->data[queue->front];
}

// Free the memory allocated for the queue
void freeQueue(Queue *queue)
{
    free(queue->data);
    initializeQueue(queue);
}