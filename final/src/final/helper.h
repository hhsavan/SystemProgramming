#ifndef HELPER_H
#define HELPER_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <complex.h>
#include <unistd.h>
#include <arpa/inet.h>

// Define constants
#define MAX_COOKS 10
#define MAX_DELIVERIES 10
#define OVEN_CAPACITY 6
#define APERTURE_COUNT 3
#define OVEN_OPENINGS 2

// Enum for order and meal status
typedef enum {
    ORDER_PLACED,
    ORDER_PREPARING,
    ORDER_IN_OVEN,
    ORDER_READY,
    ORDER_DELIVERING,
    ORDER_DELIVERED,
    ORDER_CANCELLED
} OrderStatus, MealStatus;

// Struct for a meal
typedef struct {
    MealStatus status;
    int orderPid;
    int clientId;
    double x; // Coordinates of the client
    double y;
} Meal;

// Struct for an order
typedef struct Order {
    struct Order* next;
    OrderStatus status;
    pid_t pid;
    int id;
    double townWidth;
    double townHeight;
    int numberOfClients;
    int clientSocket;
    Meal* meals;
} Order;

// Struct for a cook
typedef struct {
    pthread_t thread;
    int id;
} Cook;

// Struct for a delivery person
typedef struct {
    pthread_t thread;
    int id;
    int deliveriesCount;
} DeliveryPerson;

// Function declarations (example)
void calculate_pseudo_inverse(double complex matrix[30][40], double complex result[40][30]);
void log_order_status(Order *order);

#endif // HELPER_H
