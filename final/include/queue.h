#ifndef QUEUE_H
#define QUEUE_H

#define QUEUE_SIZE 100

// Define the structure for orders
struct Order {
    int order_id;
    char customer_name[50];
    int quantity;
};

// Define the response structure
struct Response {
    char message[100];
};

// Define the queue structure
struct OrderQueue {
    struct Order orders[QUEUE_SIZE];
    int front;
    int rear;
    int count;
};

// Function prototypes
void init_queue(struct OrderQueue *q);
void enqueue(struct OrderQueue *q, struct Order order);
struct Order dequeue(struct OrderQueue *q);
int is_queue_empty(struct OrderQueue *q);

#endif // QUEUE_H
