#include "helper.h"
#include <math.h>

// Define port and other constants
#define PORT 8080
#define QUEUE_SIZE 10
#define MAX_ORDERS 100

// Global variables
int server_fd;
struct sockaddr_in address;
int addrlen = sizeof(address);

pthread_mutex_t order_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t order_cond = PTHREAD_COND_INITIALIZER;

Order* orders = NULL;
int order_count = 0;

sem_t sem_aperture;
sem_t sem_oven_openings;
sem_t sem_oven_capacity;

void *manager_thread(void *arg);
void *cook_thread(void *arg);
void *delivery_thread(void *arg);

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <port> <CookThreadPoolSize> <DeliveryPoolSize> <k>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int cookThreadPoolSize = atoi(argv[2]);
    int deliveryThreadPoolSize = atoi(argv[3]);
    // int deliverySpeed = atoi(argv[4]); // Not used yet
    int opt = 1;

#pragma region socket
    // Initialize server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket to the address
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, QUEUE_SIZE) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
#pragma endregion
   
    printf("PideShop active, waiting for connections...\n");

    // Initialize semaphores
    sem_init(&sem_aperture, 0, APERTURE_COUNT);
    sem_init(&sem_oven_openings, 0, OVEN_OPENINGS);
    sem_init(&sem_oven_capacity, 0, OVEN_CAPACITY);

    // Create cook threads
    Cook cooks[cookThreadPoolSize];
    for (int i = 0; i < cookThreadPoolSize; i++) {
        cooks[i].id = i;
        pthread_create(&cooks[i].thread, NULL, cook_thread, (void*)&cooks[i]);
    }

    // Create delivery threads
    DeliveryPerson deliveryPersons[deliveryThreadPoolSize];
    for (int i = 0; i < deliveryThreadPoolSize; i++) {
        deliveryPersons[i].id = i;
        pthread_create(&deliveryPersons[i].thread, NULL, delivery_thread, (void*)&deliveryPersons[i]);
    }

    // Create the manager thread
    pthread_t manager;
    pthread_create(&manager, NULL, manager_thread, NULL);

    // Join the manager thread (server runs indefinitely)
    pthread_join(manager, NULL);

    // Clean up and close the server socket
    close(server_fd);
    sem_destroy(&sem_aperture);
    sem_destroy(&sem_oven_openings);
    sem_destroy(&sem_oven_capacity);
    return 0;
}

void *manager_thread(void *arg) {
    int new_socket;
    struct sockaddr_in client_address;
    int client_addrlen = sizeof(client_address);

    while (1) {
        // Accept incoming connections
        if ((new_socket = accept(server_fd, (struct sockaddr *)&client_address, (socklen_t *)&client_addrlen)) < 0) {
            perror("accept");
            continue;
        }

        printf("Connection accepted from %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

        // Receive order from client
        Order* new_order = malloc(sizeof(Order));
        recv(new_socket, new_order, sizeof(Order), 0);

        // Receive meals for the order
        new_order->meals = malloc(new_order->numberOfClients * sizeof(Meal));
        recv(new_socket, new_order->meals, new_order->numberOfClients * sizeof(Meal), 0);

        // Print received meals
        for (int i = 0; i < new_order->numberOfClients; i++) {
            printf("Server received meal %d for order %d at (%f, %f)\n",
                   i, new_order->id, new_order->meals[i].x, new_order->meals[i].y);
        }

        // Add order to the queue
        pthread_mutex_lock(&order_mutex);
        new_order->next = orders;
        orders = new_order;
        new_order->status = ORDER_PLACED;
        log_order_status(new_order);
        order_count++;
        pthread_cond_signal(&order_cond);
        pthread_mutex_unlock(&order_mutex);

        // Close the client socket
        close(new_socket);
    }

    return NULL;
}

void *cook_thread(void *arg) {
    while (1) {
        Order *order = NULL;

        // Wait for an order
        pthread_mutex_lock(&order_mutex);
        while (order_count == 0) {
            pthread_cond_wait(&order_cond, &order_mutex);
        }
        order = orders;
        orders = orders->next;
        order_count--;
        order->status = ORDER_PREPARING;
        log_order_status(order);
        pthread_mutex_unlock(&order_mutex);

        for (int i = 0; i < order->numberOfClients; i++) {
            Meal *meal = &order->meals[i];

            // Prepare the meal
            double complex matrix[30][40];
            double complex result[40][30];
            calculate_pseudo_inverse(matrix, result);
            sleep(2); // Simulate preparation time

            // Wait for an aperture and oven opening
            sem_wait(&sem_aperture);
            sem_wait(&sem_oven_openings);

            // Place the meal in the oven
            sem_wait(&sem_oven_capacity);
            meal->status = ORDER_IN_OVEN;
            log_order_status(order);
            sem_post(&sem_oven_openings);
            sem_post(&sem_aperture);

            // Simulate cooking time
            sleep(1);

            // Wait for an aperture and oven opening to take out the meal
            sem_wait(&sem_aperture);
            sem_wait(&sem_oven_openings);

            // Take the meal out of the oven
            sem_post(&sem_oven_capacity);
            meal->status = ORDER_READY;
            log_order_status(order);
            sem_post(&sem_oven_openings);
            sem_post(&sem_aperture);
        }

        // Pass the meals to the manager for delivery
        pthread_mutex_lock(&order_mutex);
        order->status = ORDER_READY;
        order->next = orders;
        orders = order;
        order_count++;
        pthread_cond_signal(&order_cond);
        pthread_mutex_unlock(&order_mutex);
    }

    return NULL;
}

void *delivery_thread(void *arg) {
    while (1) {
        Order *order = NULL;

        // Wait for a ready order
        pthread_mutex_lock(&order_mutex);
        while (order_count == 0 || orders->status != ORDER_READY) {
            pthread_cond_wait(&order_cond, &order_mutex);
        }
        order = orders;
        orders = orders->next;
        order_count--;
        order->status = ORDER_DELIVERING;
        log_order_status(order);
        pthread_mutex_unlock(&order_mutex);

        for (int i = 0; i < order->numberOfClients; i++) {
            Meal *meal = &order->meals[i];

            // Simulate delivery time
            int delivery_time = sqrt(pow(meal->x, 2) + pow(meal->y, 2)) / 1; // assuming delivery speed is 1 m/min for simplicity
            sleep(delivery_time);

            // Complete the delivery
            meal->status = ORDER_DELIVERED;
            log_order_status(order);
        }

        // Mark order as delivered
        pthread_mutex_lock(&order_mutex);
        order->status = ORDER_DELIVERED;
        log_order_status(order);
        pthread_mutex_unlock(&order_mutex);
    }

    return NULL;
}
