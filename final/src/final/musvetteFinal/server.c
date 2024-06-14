#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <complex.h>
#include <math.h>
#include <time.h>
#include "queue.h"
#include "linkedlist.h"

#define BUFFER_SIZE 1024
#define M 500
#define N 500

typedef struct
{
    int orderPid;
    int mealId;
    double x;
    double y;
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
} DeliveryData;

clock_t pseudoInverseMatrix(CookData *cookdata);

pthread_mutex_t meals_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t oven_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t delivery_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cook_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t delivery_cond = PTHREAD_COND_INITIALIZER;
sem_t meal_semaphore;
sem_t oven_semaphore;
sem_t oven_aperture_semaphore;
sem_t oven_door_semaphore;
Queue *meals_queue;
Queue *delivery_queue;
LinkedList *oven_list;

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

    while (1)
    {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }

        printf("Connection established with client\n");

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

        // Enqueue each meal to the meals queue
        pthread_mutex_lock(&meals_mutex);
        for (int i = 0; i < new_order->numberOfMeals; i++)
        {
            enqueue(meals_queue, &new_order->meals[i]);
            sem_post(&meal_semaphore);
        }
        printf("Meals received and added to queue\n");
        pthread_mutex_unlock(&meals_mutex);

        free(new_order); // Free the order struct, but not the meals, as they are in the queue
        close(new_socket);
    }
}

void ovenFunc(Meal *meal, CookData *cook, int comingPurpose)
{
    if (comingPurpose == 0)
    { // If cook come to oven for putting
        sem_wait(&oven_semaphore);
        sem_wait(&oven_aperture_semaphore);
        sem_wait(&oven_door_semaphore);

        // At this point, the cook can put the meal in the oven
        pthread_mutex_lock(&oven_mutex);
        insertAtEnd(oven_list, meal);
        cook->cookingTime = clock() + cook->preparingTime / 2;
        cook->mealInOven = meal;
        printf("Meal %d for order %d is cooking\n", meal->mealId, meal->orderPid);
        pthread_mutex_unlock(&oven_mutex);

        sem_post(&oven_door_semaphore);
        sem_post(&oven_aperture_semaphore);
        // sem_post(&oven_semaphore);
    }
    else if (comingPurpose == 1)
    { // If cook come to oven for taking
        sem_wait(&oven_aperture_semaphore);
        sem_wait(&oven_door_semaphore);

        // At this point, the cook can take the meal out of the oven
        pthread_mutex_lock(&oven_mutex);
        deleteNode(oven_list, cook->mealInOven);
        pthread_mutex_unlock(&oven_mutex);

        sem_post(&oven_door_semaphore);
        sem_post(&oven_aperture_semaphore);
        sem_post(&oven_semaphore);

        cook->meal_count++;
        printf("Cook(%d) finished meal %d for order %d\n", cook->id, cook->mealInOven->mealId, cook->mealInOven->orderPid);
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
                //printf("BURADADADADAD\n");
                ovenFunc(NULL, cook, 1);
            }
        }
        // sem_wait(&meal_semaphore);

        pthread_mutex_lock(&meals_mutex);
        Meal *meal = (Meal *)dequeue(meals_queue);
        pthread_mutex_unlock(&meals_mutex);

        if (meal == NULL)
        {
            continue;
        }

        // Simulate meal preparation time
        printf("Cook(%d) preparing meal %d for order %d\n", cook->id, meal->mealId, meal->orderPid);
        // sleep(cook->preparingTime); // Simulate preparation time
        cook->preparingTime = pseudoInverseMatrix(cook);
        ovenFunc(meal, cook, 0); // Put meal in oven

        // Simulate meal cooking time
        // sleep(cook->cookingTime); // Simulate cooking time

        // ovenFunc(meal, 1);  // Take meal out of oven

        // Enqueue meal to the delivery queue
    }
}

void *deliveryThreadFunc(void *arg)
{
    DeliveryData *data = (DeliveryData *)arg;
    while (1)
    {
        pthread_mutex_lock(&delivery_mutex);
        while (isQueueEmpty(delivery_queue))
        {
            pthread_cond_wait(&delivery_cond, &delivery_mutex);
        }
        Meal *meal = (Meal *)dequeue(delivery_queue);
        pthread_mutex_unlock(&delivery_mutex);

        if (meal == NULL)
        {
            continue;
        }

        // Simulate delivery time
        data->meal_count++;
        printf("Delivery person(%d) delivering meal %d for order %d\n", data->id, meal->mealId, meal->orderPid);
        sleep(1); // Simulate time
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
    int cookThreadPoolSize = atoi(argv[2]);
    int deliveryPoolSize = atoi(argv[3]);
    int deliverySpeed = atoi(argv[4]);

    pthread_t phone_thread, cook_threads[cookThreadPoolSize], delivery_threads[deliveryPoolSize];
    CookData cook_data[cookThreadPoolSize];
    // for (size_t i = 0; i < cookThreadPoolSize; i++)
    // {
    //     cook_data[i].mealInOven = NULL;
    // }

    DeliveryData delivery_data[deliveryPoolSize];

    sem_init(&meal_semaphore, 0, 0);
    sem_init(&oven_semaphore, 0, 6);
    sem_init(&oven_aperture_semaphore, 0, 3);
    sem_init(&oven_door_semaphore, 0, 2);

    meals_queue = createQueue();
    delivery_queue = createQueue();
    oven_list = createLinkedList();

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
    pthread_cond_destroy(&cook_cond);
    pthread_cond_destroy(&delivery_cond);

    freeQueue(meals_queue);
    freeQueue(delivery_queue);
    freeLinkedList(oven_list);

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
            // printf("asdasdasdsdasdasdasdasdasdasdasdasdasdasdasd\n");
            // printf("segfault: %d",cookdata->mealInOven->mealId);
        if (cookdata->mealInOven != NULL && (clock() >= cookdata->cookingTime))
        {
            //printf("BURADADADADAD\n");
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
    // Note: In a real scenario, these would be computed using SVD
    for (i = 0; i < M; i++)
    {
        if (cookdata->mealInOven != NULL && (clock() >= cookdata->cookingTime))
        {
            //printf("BURADADADADAD\n");
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
            //printf("BURADADADADAD\n");
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
            //printf("BURADADADADAD\n");
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
                //printf("BURADADADADAD\n");
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
            //printf("BURADADADADAD\n");
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
    double time_taken = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    // printf("Time taken to compute the pseudoinverse: %f seconds\n", time_taken);

    return end_time - start_time;
}
