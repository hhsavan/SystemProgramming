#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

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
    int id;
    double townWidth;
    double townHeight;
    int numberOfMeals;
    int clientSocket;
} Order;

void generate_random_order(Order *order, int town_size) {
    order->status = NOT_PLACED;
    order->pid = getpid();
    order->id = rand() % 1000;
    order->townWidth = town_size;
    order->townHeight = town_size;
    order->numberOfMeals = ORDER_SIZE;

    for (int i = 0; i < ORDER_SIZE; i++) {
        order->meals[i].status = NOT_PLACED;
        order->meals[i].orderPid = order->pid;
        order->meals[i].mealId = i;
        order->meals[i].x = (double)rand() / RAND_MAX * town_size;
        order->meals[i].y = (double)rand() / RAND_MAX * town_size;
    }
}

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

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s [server IP] [port] [numberOfClients] [townSize]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *server_ip = argv[1];
    int port = atoi(argv[2]);
    int number_of_clients = atoi(argv[3]);
    int town_size = atoi(argv[4]);

    srand(time(NULL));

    int sock = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    Order order;
    generate_random_order(&order, town_size);
    print_order(&order);

    // Send order to the server
    write(sock, &order, sizeof(Order));
    write(sock, order.meals, ORDER_SIZE * sizeof(Meal));

    // Wait for response from the server
    char buffer[1024] = {0};
    read(sock, buffer, 1024);
    printf("Server response: %s\n", buffer);

    close(sock);
    return 0;
}
