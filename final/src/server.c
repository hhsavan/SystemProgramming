#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_COOKS 10
#define MAX_DELIVERIES 10
#define MAX_OVEN_CAPACITY 6
#define MAX_DELIVERY_BAG_CAPACITY 3
#define ORDER_SIZE 10

typedef enum {
    NOT_PLACED,
    PLACED,
    PREPARING,
    PREPARED,
    IN_OVEN,
    COOKED,
    READY_FOR_DELIVERY,
    OUT_FOR_DELIVERY,
    DELIVERED,
    CANCEL,
    ALL_SERVED
} OrderStatus;

typedef struct {
    OrderStatus status;
    int orderPid;
    int mealId;
    double x;           /* Coordinates of the client */
    double y;
} Meal;

typedef struct {
    Meal meals[ORDER_SIZE];
    OrderStatus status;
    pid_t pid;
    int id; // Unique identifier for the order
    double townWidth;
    double townHeight;
    int numberOfMeals;
    int clientSocket;
} Order;

void print_order(const Order *order) {
    printf("Order ID: %d\n", order->id);
    printf("Order PID: %d\n", order->pid);
    printf("Town Size: %.2f x %.2f\n", order->townWidth, order->townHeight);
    printf("Number of Meals: %d\n", order->numberOfMeals);
    printf("Order Status: %d\n", order->status);

    for (int i = 0; i < ORDER_SIZE; i++) {
        printf("  Meal %d:\n", i + 1);
        printf("    Status: %d\n", order->meals[i].status);
        printf("    Order PID: %d\n", order->meals[i].orderPid);
        printf("    Meal ID: %d\n", order->meals[i].mealId);
        printf("    Coordinates: (%.2f, %.2f)\n", order->meals[i].x, order->meals[i].y);
    }
}

// Function prototypes for cook and delivery threads (implementations not provided)
void *cook_thread(void *arg);
void *delivery_thread(void *arg);

void handle_client(int client_socket) {
    Order *comingOrder = malloc(sizeof(Order));
    if (comingOrder == NULL) {
        perror("malloc failed");
        close(client_socket);
        return;
    }

    if (read(client_socket, comingOrder, sizeof(Order)) <= 0) {
        perror("Failed to read order");
        close(client_socket);
        free(comingOrder);
        return;
    }

    comingOrder->meals = malloc(ORDER_SIZE * sizeof(Meal));
    if (comingOrder->meals == NULL) {
        perror("malloc failed");
        close(client_socket);
        free(comingOrder);
        return;
    }

    if (read(client_socket, comingOrder->meals, ORDER_SIZE * sizeof(Meal)) <= 0) {
        perror("Failed to read meals");
        close(client_socket);
        free(comingOrder->meals);
        free(comingOrder);
        return;
    }

    printf("Received order from Client ID: %d\n", comingOrder->id);
    print_order(comingOrder); // Optional: Print the received order details

    // Process the order (send to cook/delivery threads)
    // ... (Implement logic to add order to shared data structure and signal cook threads)

    // Send acknowledgment to client (optional)
    // write(client_socket, "Order received", strlen("Order received"));

    close(client_socket);
    free(comingOrder->meals);
    free(comingOrder);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s [port] [maxClients]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    int max_clients = atoi(argv
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Create a socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options (optional, you can research for SO_REUSEADDR)
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Bind the socket to a specific address and port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Listen on all interfaces
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, max_clients) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", port);

    // Array or data structure to store orders (shared between threads)
    // You'll need to implement synchronization mechanisms (mutexes, semaphores)
    // to ensure safe access to this data structure.
    Order *orders[MAX_CLIENTS]; // Placeholder, replace with appropriate data structure 

    // Create cook and delivery threads
    pthread_t cook_threads[MAX_COOKS];
    pthread_t delivery_threads[MAX_DELIVERIES];
    for (int i = 0; i < MAX_COOKS; i++) {
        if (pthread_create(&cook_threads[i], NULL, cook_thread, (void *)orders) != 0) {
            perror("Failed to create cook thread");
        }
    }

    for (int i = 0; i < MAX_DELIVERIES; i++) {
        if (pthread_create(&delivery_threads[i], NULL, delivery_thread, (void *)orders) != 0) {
            perror("Failed to create delivery thread");
        }
    }

    // Main server loop: Accept connections and handle clients in separate threads
    while ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) >= 0) {
        printf("Client connected\n");
        pthread_t client_thread;
        pthread_create(&client_thread, NULL, (void *)handle_client, (void *)client_socket);
        pthread_detach(client_thread); // Detach the thread to avoid resource leaks
    }

    // Wait for cook and delivery threads to finish (optional)
    for (int i = 0; i < MAX_COOKS; i++) {
        pthread_join(cook_threads[i], NULL);
    }

    for (int i = 0; i < MAX_DELIVERIES; i++) {
        pthread_join(delivery_threads[i], NULL);
    }

    close(server_fd);
    return 0;
}

// Implement cook thread logic
void *cook_thread(void *arg) {
    // Cast the argument back to the order data structure
    Order *order_list = (Order *)arg;

    // Continuously loop
    while (1) {
        // Check for new orders in the order list (using synchronization mechanisms)
        // ...

        // If an order is found, simulate cooking process (e.g., sleep)
        // ...

        // Update order status and potentially notify delivery threads
        // ...
    }

    return NULL;
}

// Implement delivery thread logic
void *delivery_thread(void *arg) {
    // Cast the argument back to the order data structure
    Order *order_list = (Order *)arg;

    // Continuously loop
    while (1) {
        // Check for orders ready for delivery in the order list (using synchronization mechanisms)
        // ...

        // If an order is found, simulate delivery process (e
