#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>
#include <pthread.h>

// Structure for a queue node
typedef struct QueueNode {
    void* data;
    struct QueueNode* next;
} QueueNode;

// Structure for a queue
typedef struct Queue {
    QueueNode* front;
    QueueNode* rear;
    pthread_mutex_t mutex;
} Queue;

// Function to create a new queue node
QueueNode* newQueueNode(void* data);

// Function to create an empty queue
Queue* createQueue();

// Function to add an item to the queue
void enqueue(Queue* queue, void* data);

// Function to remove an item from the queue
void* dequeue(Queue* queue);

// Function to check if the queue is empty
bool isQueueEmpty(Queue* queue);

// Function to get the front item of the queue
void* front(Queue* queue);

// Function to get the rear item of the queue
void* rear(Queue* queue);

// Function to free the queue
void freeQueue(Queue* queue);

#endif
