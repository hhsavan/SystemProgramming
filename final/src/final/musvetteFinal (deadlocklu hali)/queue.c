#include "queue.h"
#include <stdlib.h>

QueueNode* newQueueNode(void* data) {
    QueueNode* node = (QueueNode*)malloc(sizeof(QueueNode));
    node->data = data;
    node->next = NULL;
    return node;
}

Queue* createQueue() {
    Queue* queue = (Queue*)malloc(sizeof(Queue));
    queue->front = queue->rear = NULL;
    pthread_mutex_init(&queue->mutex, NULL);
    return queue;
}

void enqueue(Queue* queue, void* data) {
    pthread_mutex_lock(&queue->mutex);
    QueueNode* node = newQueueNode(data);
    if (queue->rear == NULL) {
        queue->front = queue->rear = node;
    } else {
        queue->rear->next = node;
        queue->rear = node;
    }
    pthread_mutex_unlock(&queue->mutex);
}

void* dequeue(Queue* queue) {
    pthread_mutex_lock(&queue->mutex);
    if (queue->front == NULL) {
        pthread_mutex_unlock(&queue->mutex);
        return NULL;
    }
    QueueNode* node = queue->front;
    queue->front = queue->front->next;
    if (queue->front == NULL) {
        queue->rear = NULL;
    }
    void* data = node->data;
    free(node);
    pthread_mutex_unlock(&queue->mutex);
    return data;
}

bool isQueueEmpty(Queue* queue) {
    pthread_mutex_lock(&queue->mutex);
    bool empty = (queue->front == NULL);
    pthread_mutex_unlock(&queue->mutex);
    return empty;
}

void* front(Queue* queue) {
    pthread_mutex_lock(&queue->mutex);
    if (queue->front == NULL) {
        pthread_mutex_unlock(&queue->mutex);
        return NULL;
    }
    void* data = queue->front->data;
    pthread_mutex_unlock(&queue->mutex);
    return data;
}

void* rear(Queue* queue) {
    pthread_mutex_lock(&queue->mutex);
    if (queue->rear == NULL) {
        pthread_mutex_unlock(&queue->mutex);
        return NULL;
    }
    void* data = queue->rear->data;
    pthread_mutex_unlock(&queue->mutex);
    return data;
}

void freeQueue(Queue* queue) {
    pthread_mutex_lock(&queue->mutex);
    QueueNode* node = queue->front;
    while (node != NULL) {
        QueueNode* temp = node;
        node = node->next;
        free(temp);
    }
    pthread_mutex_unlock(&queue->mutex);
    pthread_mutex_destroy(&queue->mutex);
    free(queue);
}
