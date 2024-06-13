#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include "../include/queue.h"   

#define PORT 8080
#define MAX_CLIENTS 10

// Queue to store orders
struct OrderQueue order_queue;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;

// Array to store client sockets
int client_sockets[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;

void *client_handler(void *socket_desc) {
    int sock = *(int*)socket_desc;
    struct Order order;
    struct Response response;

    // Receive the struct from the client
    read(sock, &order, sizeof(order));

    // Lock the queue and add the order
    pthread_mutex_lock(&queue_mutex);
    enqueue(&order_queue, order);
    pthread_cond_signal(&queue_cond);
    pthread_mutex_unlock(&queue_mutex);

    // Prepare the response
    snprintf(response.message, sizeof(response.message), "Order ID %d received successfully", order.order_id);

    // Send the response to the client
    send(sock, &response, sizeof(response), 0);

    // Add the client socket to the list
    pthread_mutex_lock(&client_mutex);
    client_sockets[client_count++] = sock;
    pthread_mutex_unlock(&client_mutex);

    return 0;
}

void *order_manager(void *arg) {
    struct Order order;
    struct Response response;
    time_t t;

    while (1) {
        // Wait for an order to be placed in the queue
        pthread_mutex_lock(&queue_mutex);
        while (is_queue_empty(&order_queue)) {
            pthread_cond_wait(&queue_cond, &queue_mutex);
        }
        order = dequeue(&order_queue);
        pthread_mutex_unlock(&queue_mutex);

        // Prepare the response
        t = time(NULL);
        snprintf(response.message, sizeof(response.message), "Order ID %d is being processed. Server time: %s", order.order_id, ctime(&t));

        // Inform all clients that the order is being processed
        pthread_mutex_lock(&client_mutex);
        for (int i = 0; i < client_count; i++) {
            send(client_sockets[i], &response, sizeof(response), 0);
        }
        pthread_mutex_unlock(&client_mutex);

        // Simulate order processing time
        sleep(5);
    }

    return 0;
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    pthread_t thread_id, manager_thread;

    // Initialize the queue
    init_queue(&order_queue);

    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Bind the socket to the network address and port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d\n", PORT);

    // Create a thread to manage orders
    pthread_create(&manager_thread, NULL, order_manager, NULL);

    while ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) >= 0) {
        // Create a thread for each new client
        if (pthread_create(&thread_id, NULL, client_handler, (void*)&new_socket) < 0) {
            perror("could not create thread");
            return 1;
        }

        printf("Handler assigned\n");
    }

    if (new_socket < 0) {
        perror("accept failed");
        return 1;
    }

    return 0;
}
