#include "./include/queue.h"

// Function to initialize the queue
void init_queue(struct OrderQueue *q) {
    q->front = 0;
    q->rear = -1;
    q->count = 0;
}

// Function to enqueue an order
void enqueue(struct OrderQueue *q, struct Order order) {
    if (q->count < QUEUE_SIZE) {
        q->rear = (q->rear + 1) % QUEUE_SIZE;
        q->orders[q->rear] = order;
        q->count++;
    }
}

// Function to dequeue an order
struct Order dequeue(struct OrderQueue *q) {
    struct Order order = {0};
    if (q->count > 0) {
        order = q->orders[q->front];
        q->front = (q->front + 1) % QUEUE_SIZE;
        q->count--;
    }
    return order;
}

// Function to check if the queue is empty
int is_queue_empty(struct OrderQueue *q) {
    return q->count == 0;
}
