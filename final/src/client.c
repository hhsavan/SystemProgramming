#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define NUM_CLIENTS 10

// Define the structure
struct Order {
    int order_id;
    char customer_name[50];
    int quantity;
};

// Define the response structure
struct Response {
    char message[100];
};

void *client_thread(void *arg) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    struct Order order;
    struct Response response;

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return NULL;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return NULL;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return NULL;
    }

    // Fill the struct with data
    order.order_id = pthread_self();
    strcpy(order.customer_name, "John Doe");
    order.quantity = 5;

    // Send the struct to the server
    send(sock, &order, sizeof(order), 0);
    printf("Order sent to the server from thread %ld\n", pthread_self());

    // Client loop to wait for server messages
    while (1) {
        // Receive the response from the server
        if (read(sock, &response, sizeof(response)) > 0) {
            printf("Server Response to thread %ld: %s\n", pthread_self(), response.message);
        } else {
            printf("Server connection closed for thread %ld\n", pthread_self());
            break;
        }
    }

    // Close the socket
    close(sock);
    return NULL;
}

int main() {
    pthread_t clients[NUM_CLIENTS];

    // Create client threads
    for (int i = 0; i < NUM_CLIENTS; i++) {
        if (pthread_create(&clients[i], NULL, client_thread, NULL) != 0) {
            perror("Failed to create client thread");
        }
    }

    // Wait for all threads to finish
    for (int i = 0; i < NUM_CLIENTS; i++) {
        pthread_join(clients[i], NULL);
    }

    return 0;
}
