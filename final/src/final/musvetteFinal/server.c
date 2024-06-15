#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <complex.h>
#include <math.h>
#include <time.h>
#include <signal.h>
#include "queue.h"
#include "linkedlist.h"

#define BUFFER_SIZE 1024
#define M 30
#define N 40
#define MAX_DELIVERY_CAPACITY 3

typedef enum
{
    PLACED,
    PREPARED,
    COOKED,
    IN_DELIVERY,
    DELIVERED,
    CANCELED,
    SERVERDOWN
} MealStatus;

typedef struct
{
    int orderPid;
    int mealId;
    double x;
    double y;
    MealStatus status;
    int clientSocket;
} Meal;

typedef struct
{
    int pid;
    int id;
    double townWidth;
    double townHeight;
    int numberOfMeals;
    int clientSocket;
    Meal *meals; // Pointer to meals array
} Order;

void printOrder(const Order *order)
{
    printf("Order ID: %d\n", order->id);
    printf("PID: %d\n", order->pid);
    printf("Town Width: %.2f, Town Height: %.2f\n", order->townWidth, order->townHeight);
    printf("Number of Meals: %d\n", order->numberOfMeals);
    printf("Client Socket: %d\n", order->clientSocket);
    for (int i = 0; i < order->numberOfMeals; i++)
    {
        printf("  Meal %d:\n", i + 1);
        printf("    Order PID: %d\n", order->meals[i].orderPid);
        printf("    Meal ID: %d\n", order->meals[i].mealId);
        printf("    Coordinates: (%.2f, %.2f)\n", order->meals[i].x, order->meals[i].y);
        printf("    Status: %d\n", order->meals[i].status);
    }
}

typedef struct
{
    int id;
    int meal_count;
    clock_t preparingTime;
    clock_t cookingTime;
    Meal *mealInOven;
} CookData;

typedef struct
{
    int id;
    int meal_count;
    Meal *meals[MAX_DELIVERY_CAPACITY];
    int meal_index;
} DeliveryData;

double shopX; // Assuming the shop is in the middle of the map (5/2, 5/2)
double shopY;

clock_t pseudoInverseMatrix(CookData *cookdata);

pthread_mutex_t meals_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t oven_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t delivery_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t delivery_mutex_for_bag = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cooked_meals_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t all_meals_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cook_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t delivery_cond = PTHREAD_COND_INITIALIZER;
sem_t meal_semaphore;
sem_t oven_semaphore;
sem_t oven_aperture_semaphore;
sem_t oven_door_semaphore;
Queue *meals_queue;
Queue *delivery_queue;
LinkedList *oven_list;
LinkedList *all_meals_list;

int total_meals_received = 0;
int total_meals_cooked = 0;
int deliverySpeed = 0;

pthread_t phone_thread;
pthread_t *cook_threads;
pthread_t *delivery_threads;
CookData *cook_data;
DeliveryData *delivery_data;
int cookThreadPoolSize;
int deliveryPoolSize;
void handle_sigpipe(int signum)
{
    // printf("SIGPIPE caught\n");
}


const char *logFileName = "./server_log.txt";
int logFile;
void logStatus(const char *status)
{
    pthread_mutex_lock(&log_mutex);
    dprintf(logFile, "%s\n", status);
    pthread_mutex_unlock(&log_mutex);
}

void handle_sigint(int sig)
{
    printf("SIGINT caught, shutting down server...\n");
    logStatus("SIGINT caught, shutting down server...");
    // Set all ongoing meals to SERVERDOWN
    pthread_mutex_lock(&all_meals_mutex);
    ListNode *node = all_meals_list->head;
    while (node != NULL)
    {
        Meal *meal = (Meal *)node->data;
        if (meal->status != DELIVERED && meal->status != CANCELED)
        {
            meal->status = SERVERDOWN;
            send(meal->clientSocket, meal, sizeof(Meal), 0);
        }
        node = node->next;
    }
    pthread_mutex_unlock(&all_meals_mutex);

    // Cleanup resources
    pthread_cancel(phone_thread);
    for (int i = 0; i < cookThreadPoolSize; i++)    pthread_cancel(cook_threads[i]);
    
    for (int i = 0; i < deliveryPoolSize; i++)      pthread_cancel(delivery_threads[i]);

    sem_destroy(&meal_semaphore);
    sem_destroy(&oven_semaphore);
    sem_destroy(&oven_aperture_semaphore);
    sem_destroy(&oven_door_semaphore);
    pthread_mutex_destroy(&meals_mutex);
    pthread_mutex_destroy(&oven_mutex);
    pthread_mutex_destroy(&delivery_mutex);
    pthread_mutex_destroy(&delivery_mutex_for_bag);
    pthread_mutex_destroy(&cooked_meals_mutex);
    pthread_mutex_destroy(&all_meals_mutex);
    pthread_cond_destroy(&cook_cond);
    pthread_cond_destroy(&delivery_cond);

    freeQueue(meals_queue);
    freeQueue(delivery_queue);
    freeLinkedList(oven_list);
    freeLinkedList(all_meals_list);

    free(cook_threads);
    free(delivery_threads);
    free(cook_data);
    free(delivery_data);

    exit(0);
}

void *phonethreadfunc(void *arg)
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    int portnumber = *(int *)arg;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(portnumber);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("PideShop active, waiting for connection...\n");
    logStatus("PideShop active, waiting for connection...");

    while (1)
    {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }

        printf("Connection established with client\n");
        logStatus("Connection established with client");

        // Read the order header first
        Order temp_order;
        if (recv(new_socket, &temp_order, sizeof(Order), 0) <= 0)
        {
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
        while (total_received < meals_size)
        {
            ssize_t received = recv(new_socket, (char *)new_order->meals + total_received, meals_size - total_received, 0);
            if (received <= 0)
            {
                perror("Failed to receive meals");
                free(new_order->meals);
                free(new_order);
                close(new_socket);
                continue;
            }
            total_received += received;
        }
        shopX = new_order->townWidth / 2;
        shopY = new_order->townHeight / 2;

        if (new_order->meals[0].status == CANCELED)
        {
            pthread_mutex_lock(&all_meals_mutex);
            ListNode *node = all_meals_list->head;
            while (node != NULL)
            {
                Meal *meal = (Meal *)node->data;
                if (meal->orderPid == new_order->pid && meal->status != DELIVERED)
                {
                    meal->status = CANCELED;
                }
                node = node->next;
            }
            pthread_mutex_unlock(&all_meals_mutex);
            printf("Order canceled, PID %d\n", new_order->pid);
            char logMsg[256];
            snprintf(logMsg, sizeof(logMsg), "Order canceled, PID %d", new_order->pid);
            logStatus(logMsg);

            free(new_order->meals);
            free(new_order);
            close(new_socket);
            continue;
        }

        // Enqueue each meal to the meals queue and add to all meals list
        pthread_mutex_lock(&meals_mutex);
        pthread_mutex_lock(&all_meals_mutex);
        for (int i = 0; i < new_order->numberOfMeals; i++)
        {
            new_order->meals[i].status = PLACED;
            new_order->meals[i].clientSocket = new_socket;
            enqueue(meals_queue, &new_order->meals[i]);
            insertAtEnd(all_meals_list, &new_order->meals[i]);
            sem_post(&meal_semaphore);
            total_meals_received++;
        }
        printf("Meals received and added to queue\n");
        logStatus("Meals received and added to queue");
        pthread_mutex_unlock(&all_meals_mutex);
        pthread_mutex_unlock(&meals_mutex);

        free(new_order); // Free the order struct, but not the meals, as they are in the queue
    }
}

void BestPerson(DeliveryData *deliveryPeople, CookData *cooks)
{
    int maxDeliveries = 0;
    int bestDeliveryPerson = -1;

    for (int i = 0; i < deliveryPoolSize; i++)
    {
        if (deliveryPeople[i].meal_count > maxDeliveries)
        {
            maxDeliveries = deliveryPeople[i].meal_count;
            bestDeliveryPerson = deliveryPeople[i].id;
        }
    }

    int maxCookedMeals = 0;
    int bestCookPerson = -1;

    for (int i = 0; i < cookThreadPoolSize; i++)
    {
        if (cooks[i].meal_count > maxCookedMeals)
        {
            maxCookedMeals = cooks[i].meal_count;
            bestCookPerson = cooks[i].id;
        }
    }

    if (bestCookPerson != -1 && bestDeliveryPerson != -1)
    {
        char logMsg[256];
        snprintf(logMsg, sizeof(logMsg), "Thanks Cook %d Moto %d", bestCookPerson, bestDeliveryPerson);
        logStatus(logMsg);
        printf("%s\n", logMsg);
    }
}

int safe_send(int client_socket, const void *buffer, size_t length, int flags)
{
    ssize_t bytes_sent = send(client_socket, buffer, length, flags);
    if (bytes_sent < 0)
    {
        perror("Failed to send data");
        return -1;
    }
    return 0;
}

void sendMealStatus(Meal *meal)
{
    if (safe_send(meal->clientSocket, meal, sizeof(Meal), 0) < 0)
    {
        printf("Skipping sending meal status due to previous error.\n");
        logStatus("Skipping sending meal status due to previous error.");
    }
}

void ovenFunc(Meal *meal, CookData *cook, int comingPurpose)
{
    if (comingPurpose == 0)
    { // If cook come to oven for putting
        if (meal->status == CANCELED)
            return;

        sem_wait(&oven_semaphore);
        sem_wait(&oven_aperture_semaphore);
        sem_wait(&oven_door_semaphore);

        // At this point, the cook can put the meal in the oven
        pthread_mutex_lock(&oven_mutex);
        insertAtEnd(oven_list, meal);
        cook->cookingTime = clock() + cook->preparingTime / 2;
        cook->mealInOven = meal;
        printf("Meal %d for order %d is cooking\n", meal->mealId, meal->orderPid);
        char logMsg[256];
        snprintf(logMsg, sizeof(logMsg), "Meal %d for order %d is cooking", meal->mealId, meal->orderPid);
        logStatus(logMsg);
        pthread_mutex_unlock(&oven_mutex);

        sem_post(&oven_door_semaphore);
        sem_post(&oven_aperture_semaphore);
    }
    else if (comingPurpose == 1)
    { // If cook come to oven for taking
        if (cook->mealInOven->status == CANCELED)
            return;
        sem_wait(&oven_aperture_semaphore);
        sem_wait(&oven_door_semaphore);

        // At this point, the cook can take the meal out of the oven
        pthread_mutex_lock(&oven_mutex);
        deleteNode(oven_list, cook->mealInOven);
        pthread_mutex_unlock(&oven_mutex);

        sem_post(&oven_door_semaphore);
        sem_post(&oven_aperture_semaphore);
        sem_post(&oven_semaphore);

        cook->mealInOven->status = COOKED;
        sendMealStatus(cook->mealInOven);
        cook->meal_count++;
        pthread_mutex_lock(&cooked_meals_mutex);
        total_meals_cooked++;
        pthread_mutex_unlock(&cooked_meals_mutex);
        printf("Cook(%d) finished meal %d for order %d\n", cook->id, cook->mealInOven->mealId, cook->mealInOven->orderPid);

        char logMsg[256];
        snprintf(logMsg, sizeof(logMsg), "Cook(%d) finished meal %d for order %d", cook->id, cook->mealInOven->mealId, cook->mealInOven->orderPid);
        logStatus(logMsg);

        pthread_mutex_lock(&delivery_mutex);
        enqueue(delivery_queue, cook->mealInOven);
        pthread_cond_signal(&delivery_cond);
        pthread_mutex_unlock(&delivery_mutex);
        cook->mealInOven = NULL;
    }
}

void *cookThreadFunc(void *arg)
{
    CookData *cook = (CookData *)arg;
    while (1)
    {
        while (sem_trywait(&meal_semaphore))
        {
            if (cook->mealInOven != NULL && (clock() >= cook->cookingTime))
            {
                ovenFunc(NULL, cook, 1);
            }
        }

        pthread_mutex_lock(&meals_mutex);
        Meal *meal = (Meal *)dequeue(meals_queue);
        pthread_mutex_unlock(&meals_mutex);

        if (meal == NULL)
        {
            continue;
        }

        if (meal->status == CANCELED)
        {
            continue;
        }

        // Simulate meal preparation time
        printf("Cook(%d) preparing meal %d for order %d\n", cook->id, meal->mealId, meal->orderPid);
        char logMsg[256];
        snprintf(logMsg, sizeof(logMsg), "Cook(%d) preparing meal %d for order %d", cook->id, meal->mealId, meal->orderPid);
        logStatus(logMsg);

        cook->preparingTime = pseudoInverseMatrix(cook);
        pthread_mutex_lock(&all_meals_mutex);
        if (meal->status == CANCELED)
        {
            continue;
        }
        meal->status = PREPARED;
        pthread_mutex_unlock(&all_meals_mutex);
        sendMealStatus(meal);
        ovenFunc(meal, cook, 0); // Put meal in oven
    }
}

double calculateDistance(double x1, double y1, double x2, double y2)
{
    return sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));
}

void *deliveryThreadFunc(void *arg)
{
    DeliveryData *data = (DeliveryData *)arg;

    while (1)
    {
        data->meal_index = 0;

        pthread_mutex_lock(&delivery_mutex_for_bag);
        while (data->meal_index < MAX_DELIVERY_CAPACITY)
        {
            if (total_meals_received == total_meals_cooked && !isQueueEmpty(delivery_queue))
            {
                Meal *meal = (Meal *)dequeue(delivery_queue);
                data->meals[data->meal_index++] = meal;
                break;
            }
            else if (!isQueueEmpty(delivery_queue))
            {
                Meal *meal = (Meal *)dequeue(delivery_queue);
                data->meals[data->meal_index++] = meal;
            }
            else
            {
                pthread_cond_wait(&delivery_cond, &delivery_mutex);
            }
        }
        pthread_mutex_unlock(&delivery_mutex_for_bag);

        for (int i = 0; i < data->meal_index; i++)
        {
            Meal *meal = data->meals[i];
            if (meal->status == CANCELED)
            {
                printf("Delivery person(%d) skipping canceled meal %d for order %d\n", data->id, meal->mealId, meal->orderPid);
                char logMsg[256];
                snprintf(logMsg, sizeof(logMsg), "Delivery person(%d) skipping canceled meal %d for order %d", data->id, meal->mealId, meal->orderPid);
                logStatus(logMsg);

                continue;
            }

            double distanceToCustomer = calculateDistance(shopX, shopY, meal->x, meal->y);
            double distanceBackToShop = calculateDistance(meal->x, meal->y, shopX, shopY);
            double totalDistance = distanceToCustomer + distanceBackToShop;

            // Simulate delivery time
            pthread_mutex_lock(&all_meals_mutex);
            meal->status = IN_DELIVERY;
            pthread_mutex_unlock(&all_meals_mutex);
            sendMealStatus(meal);
            data->meal_count++;
            printf("Delivery person(%d) delivering meal %d for order %d\n", data->id, meal->mealId, meal->orderPid);
            char logMsg[256];
            snprintf(logMsg, sizeof(logMsg), "Delivery person(%d) delivering meal %d for order %d", data->id, meal->mealId, meal->orderPid);
            logStatus(logMsg);
            sleep(totalDistance / deliverySpeed);

            if (meal->status == CANCELED)
            {
                continue;
            }

            pthread_mutex_lock(&all_meals_mutex);
            meal->status = DELIVERED;
            pthread_mutex_unlock(&all_meals_mutex);
            sendMealStatus(meal);
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 5)
    {
        fprintf(stderr, "Usage: %s [portnumber] [CookthreadPoolSize] [DeliveryPoolSize] [k]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int portnumber = atoi(argv[1]);
    cookThreadPoolSize = atoi(argv[2]);
    deliveryPoolSize = atoi(argv[3]);
    deliverySpeed = atoi(argv[4]);

    logFile = open("Log_PideShop.txt", O_CREAT | O_WRONLY | O_APPEND, 0777);
    if (logFile == -1)
    {
        perror("Failed to open PideShop logfile");
        exit(EXIT_FAILURE);
    }

    struct sigaction sa;
    sa.sa_handler = handle_sigpipe;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGPIPE, &sa, NULL);


    struct sigaction sa2;
    sa2.sa_handler = handle_sigint;
    sigemptyset(&sa2.sa_mask);
    sa2.sa_flags = 0;
    sigaction(SIGINT, &sa2, NULL);

    // signal(SIGINT, handle_sigint);

    cook_threads = malloc(cookThreadPoolSize * sizeof(pthread_t));
    delivery_threads = malloc(deliveryPoolSize * sizeof(pthread_t));
    cook_data = malloc(cookThreadPoolSize * sizeof(CookData));
    delivery_data = malloc(deliveryPoolSize * sizeof(DeliveryData));

    sem_init(&meal_semaphore, 0, 0);
    sem_init(&oven_semaphore, 0, 6);
    sem_init(&oven_aperture_semaphore, 0, 3);
    sem_init(&oven_door_semaphore, 0, 2);

    meals_queue = createQueue();
    delivery_queue = createQueue();
    oven_list = createLinkedList();
    all_meals_list = createLinkedList();

    pthread_create(&phone_thread, NULL, phonethreadfunc, &portnumber);

    for (int i = 0; i < cookThreadPoolSize; i++)
    {
        cook_data[i].id = i + 1;
        cook_data[i].meal_count = 0;
        cook_data[i].preparingTime = 1; // Example preparation time
        cook_data[i].cookingTime = 1;   // Example cooking time
        cook_data[i].mealInOven = NULL;
        pthread_create(&cook_threads[i], NULL, cookThreadFunc, &cook_data[i]);
    }

    for (int i = 0; i < deliveryPoolSize; i++)
    {
        delivery_data[i].id = i + 1;
        delivery_data[i].meal_count = 0;
        pthread_create(&delivery_threads[i], NULL, deliveryThreadFunc, &delivery_data[i]);
    }

    pthread_join(phone_thread, NULL);

    for (int i = 0; i < cookThreadPoolSize; i++)
    {
        pthread_join(cook_threads[i], NULL);
    }

    for (int i = 0; i < deliveryPoolSize; i++)
    {
        pthread_join(delivery_threads[i], NULL);
    }

    sem_destroy(&meal_semaphore);
    sem_destroy(&oven_semaphore);
    sem_destroy(&oven_aperture_semaphore);
    sem_destroy(&oven_door_semaphore);
    pthread_mutex_destroy(&meals_mutex);
    pthread_mutex_destroy(&oven_mutex);
    pthread_mutex_destroy(&delivery_mutex);
    pthread_mutex_destroy(&delivery_mutex_for_bag);
    pthread_mutex_destroy(&cooked_meals_mutex);
    pthread_mutex_destroy(&all_meals_mutex);
    pthread_cond_destroy(&cook_cond);
    pthread_cond_destroy(&delivery_cond);

    freeQueue(meals_queue);
    freeQueue(delivery_queue);
    freeLinkedList(oven_list);
    freeLinkedList(all_meals_list);

    free(cook_threads);
    free(delivery_threads);
    free(cook_data);
    free(delivery_data);

    return 0;
}

clock_t pseudoInverseMatrix(CookData *cookdata)
{
    int i, j;
    srand(time(NULL)); // Seed for random number generation

    // Start time measurement
    clock_t start_time = clock();

    // Allocate memory for the matrix
    complex double *A = (complex double *)malloc(M * N * sizeof(complex double));

    // Initialize the matrix with random non-zero complex numbers
    for (i = 0; i < M; i++)
    {
        if (cookdata->mealInOven != NULL && (clock() >= cookdata->cookingTime))
        {
            ovenFunc(NULL, cookdata, 1);
        }
        for (j = 0; j < N; j++)
        {
            double real_part = (double)(rand() % 100 + 1); // Random real part between 1 and 100
            double imag_part = (double)(rand() % 100 + 1); // Random imaginary part between 1 and 100
            A[i * N + j] = real_part + imag_part * I;
        }
    }

    // For simplicity, we assume that we have computed the SVD and have the components U, S, and V*
    // Here we manually initialize these components for demonstration purposes
    complex double *U = (complex double *)malloc(M * M * sizeof(complex double));
    complex double *S = (complex double *)malloc(M * sizeof(complex double)); // Singular values are real
    complex double *V = (complex double *)malloc(N * N * sizeof(complex double));

    // Initialize U, S, and V with example values
    for (i = 0; i < M; i++)
    {
        if (cookdata->mealInOven != NULL && (clock() >= cookdata->cookingTime))
        {
            ovenFunc(NULL, cookdata, 1);
        }
        for (j = 0; j < M; j++)
        {
            U[i * M + j] = (i == j) ? 1.0 + 0.0 * I : 0.0 + 0.0 * I;
        }
    }

    for (i = 0; i < M; i++)
    {
        S[i] = 1.0 / (i + 1.0); // Example singular values
    }

    for (i = 0; i < N; i++)
    {
        if (cookdata->mealInOven != NULL && (clock() >= cookdata->cookingTime))
        {
            ovenFunc(NULL, cookdata, 1);
        }
        for (j = 0; j < N; j++)
        {
            V[i * N + j] = (i == j) ? 1.0 + 0.0 * I : 0.0 + 0.0 * I;
        }
    }

    // Compute the pseudo-inverse A_pinv = V * S_pinv * U*
    complex double *A_pinv = (complex double *)malloc(N * M * sizeof(complex double));

    // Compute S_pinv
    complex double *S_pinv = (complex double *)malloc(M * N * sizeof(complex double));
    for (i = 0; i < N; i++)
    {
        if (cookdata->mealInOven != NULL && (clock() >= cookdata->cookingTime))
        {
            ovenFunc(NULL, cookdata, 1);
        }
        for (j = 0; j < M; j++)
        {
            if (i == j && i < M)
            {
                S_pinv[i * M + j] = 1.0 / S[i];
            }
            else
            {
                S_pinv[i * M + j] = 0.0 + 0.0 * I;
            }
        }
    }

    // Compute V * S_pinv
    complex double *VS_pinv = (complex double *)malloc(N * M * sizeof(complex double));
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < M; j++)
        {
            if (cookdata->mealInOven != NULL && (clock() >= cookdata->cookingTime))
            {
                ovenFunc(NULL, cookdata, 1);
            }
            VS_pinv[i * M + j] = 0.0 + 0.0 * I;
            for (int k = 0; k < M; k++)
            {
                VS_pinv[i * M + j] += V[i * N + k] * S_pinv[k * M + j];
            }
        }
    }

    // Compute A_pinv = (V * S_pinv) * U*
    for (i = 0; i < N; i++)
    {
        if (cookdata->mealInOven != NULL && (clock() >= cookdata->cookingTime))
        {
            ovenFunc(NULL, cookdata, 1);
        }
        for (j = 0; j < M; j++)
        {
            A_pinv[i * M + j] = 0.0 + 0.0 * I;
            for (int k = 0; k < M; k++)
            {
                A_pinv[i * M + j] += VS_pinv[i * M + k] * conj(U[j * M + k]);
            }
        }
    }

    // Free allocated memory
    free(A);
    free(U);
    free(S);
    free(V);
    free(S_pinv);
    free(VS_pinv);
    free(A_pinv);

    // End time measurement
    clock_t end_time = clock();

    // Calculate the time taken
    // double time_taken = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    // printf("Time taken to compute the pseudoinverse: %f seconds\n", time_taken);

    return end_time - start_time;
}
