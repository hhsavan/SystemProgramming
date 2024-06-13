#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include "queue.h"
#include "linkedlist.h"

#define BUFFER_SIZE 1024

typedef struct {
    int orderPid;
    int mealId;
    double x;
    double y;
} Meal;

typedef struct {
    int pid;
    int id;
    double townWidth;
    double townHeight;
    int numberOfMeals;
    int clientSocket;
    Meal *meals;  // Pointer to meals array
} Order;

void printOrder(const Order *order) {
    printf("Order ID: %d\n", order->id);
    printf("PID: %d\n", order->pid);
    printf("Town Width: %.2f, Town Height: %.2f\n", order->townWidth, order->townHeight);
    printf("Number of Meals: %d\n", order->numberOfMeals);
    printf("Client Socket: %d\n", order->clientSocket);
    for (int i = 0; i < order->numberOfMeals; i++) {
        printf("  Meal %d:\n", i + 1);
        printf("    Order PID: %d\n", order->meals[i].orderPid);
        printf("    Meal ID: %d\n", order->meals[i].mealId);
        printf("    Coordinates: (%.2f, %.2f)\n", order->meals[i].x, order->meals[i].y);
    }
}

pthread_mutex_t orders_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t oven_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cook_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t delivery_cond = PTHREAD_COND_INITIALIZER;
sem_t meal_semaphore;
sem_t oven_semaphore;
Queue *meals_queue;
LinkedList *oven_list;

void* phonethreadfunc(void* arg) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    int portnumber = *(int*)arg;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(portnumber);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("PideShop active, waiting for connection...\n");

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }

        printf("Connection established with client\n");

        // Read the order header first
        Order temp_order;
        if (recv(new_socket, &temp_order, sizeof(Order), 0) <= 0) {
            perror("Failed to receive order header");
            close(new_socket);
            continue;
        }

        // Allocate memory for the full order including meals
        Order *new_order = malloc(sizeof(Order));
        memcpy(new_order, &temp_order, sizeof(Order)); // Copy the order details
        new_order->meals = malloc(new_order->numberOfMeals * sizeof(Meal));

        // Receive the meals array
        size_t meals_size = new_order->numberOfMeals * sizeof(Meal);
        size_t total_received = 0;
        while (total_received < meals_size) {
            ssize_t received = recv(new_socket, (char*)new_order->meals + total_received, meals_size - total_received, 0);
            if (received <= 0) {
                perror("Failed to receive meals");
                free(new_order->meals);
                free(new_order);
                close(new_socket);
                continue;
            }
            total_received += received;
        }

        printOrder(new_order); // Print the received order

        // Enqueue each meal to the meals queue
        pthread_mutex_lock(&orders_mutex);
        for (int i = 0; i < new_order->numberOfMeals; i++) {
            enqueue(meals_queue, &new_order->meals[i]);
        }
        printf("Meals received and added to queue\n");
        pthread_mutex_unlock(&orders_mutex);

        for (int i = 0; i < new_order->numberOfMeals; i++) {
            sem_post(&meal_semaphore);
        }

        free(new_order); // Free the order struct, but not the meals, as they are in the queue
        close(new_socket);
    }
}

void* cookThreadFunc(void* arg) {
    while (1) {
        sem_wait(&meal_semaphore);

        pthread_mutex_lock(&orders_mutex);
        Meal *meal = (Meal *)dequeue(meals_queue);
        pthread_mutex_unlock(&orders_mutex);

        if (meal == NULL) {
            continue;
        }

        // Simulate meal preparation time
        printf("Cook preparing meal %d for order %d\n", meal->mealId, meal->orderPid);
        sleep(1); // Simulate time

        sem_wait(&oven_semaphore);

        pthread_mutex_lock(&oven_mutex);
        // Simulate putting meal in oven
        printf("Cook putting meal %d for order %d in oven\n", meal->mealId, meal->orderPid);
        insertAtEnd(oven_list, meal);
        sleep(1); // Simulate time
        pthread_mutex_unlock(&oven_mutex);

        sem_post(&oven_semaphore);

        // Simulate meal cooking time
        printf("Meal %d for order %d is cooking\n", meal->mealId, meal->orderPid);
        sleep(1); // Simulate time

        pthread_mutex_lock(&oven_mutex);
        // Simulate taking meal out of oven
        printf("Cook taking meal %d for order %d out of oven\n", meal->mealId, meal->orderPid);
        deleteNode(oven_list, meal);
        pthread_mutex_unlock(&oven_mutex);

        // Notify manager
        printf("Cook finished meal %d for order %d\n", meal->mealId, meal->orderPid);
        pthread_cond_signal(&delivery_cond);
    }
}

void* managerThreadFunc(void* arg) {
    while (1) {
        pthread_mutex_lock(&orders_mutex);
        while (isQueueEmpty(meals_queue)) {
            pthread_cond_wait(&cook_cond, &orders_mutex);
        }
        pthread_mutex_unlock(&orders_mutex);
    }
}

void* deliveryThreadFunc(void* arg) {
    while (1) {
        pthread_mutex_lock(&orders_mutex);
        while (isQueueEmpty(meals_queue)) {
            pthread_cond_wait(&delivery_cond, &orders_mutex);
        }
        Meal *meal = (Meal *)dequeue(meals_queue);
        pthread_mutex_unlock(&orders_mutex);

        if (meal == NULL) {
            continue;
        }

        // Simulate delivery time
        printf("Delivery person delivering meal %d for order %d\n", meal->mealId, meal->orderPid);
        sleep(1); // Simulate time
    }
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s [portnumber] [CookthreadPoolSize] [DeliveryPoolSize] [k]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int portnumber = atoi(argv[1]);
    int cookThreadPoolSize = atoi(argv[2]);
    int deliveryPoolSize = atoi(argv[3]);
    int deliverySpeed = atoi(argv[4]);

    pthread_t phone_thread, manager_thread, cook_threads[cookThreadPoolSize], delivery_threads[deliveryPoolSize];

    sem_init(&meal_semaphore, 0, 0);
    sem_init(&oven_semaphore, 0, 6);

    meals_queue = createQueue();
    oven_list = createLinkedList();

    pthread_create(&phone_thread, NULL, phonethreadfunc, &portnumber);
    pthread_create(&manager_thread, NULL, managerThreadFunc, NULL);

    for (int i = 0; i < cookThreadPoolSize; i++) {
        pthread_create(&cook_threads[i], NULL, cookThreadFunc, NULL);
    }

    for (int i = 0; i < deliveryPoolSize; i++) {
        pthread_create(&delivery_threads[i], NULL, deliveryThreadFunc, NULL);
    }

    pthread_join(phone_thread, NULL);
    pthread_join(manager_thread, NULL);

    for (int i = 0; i < cookThreadPoolSize; i++) {
        pthread_join(cook_threads[i], NULL);
    }

    for (int i = 0; i < deliveryPoolSize; i++) {
        pthread_join(delivery_threads[i], NULL);
    }

    sem_destroy(&meal_semaphore);
    sem_destroy(&oven_semaphore);
    pthread_mutex_destroy(&orders_mutex);
    pthread_mutex_destroy(&oven_mutex);
    pthread_cond_destroy(&cook_cond);
    pthread_cond_destroy(&delivery_cond);

    freeQueue(meals_queue);
    freeLinkedList(oven_list);

    return 0;
}
