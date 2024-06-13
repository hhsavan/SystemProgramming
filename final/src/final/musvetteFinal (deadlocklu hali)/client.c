#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

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

int connect_to_server(int portnumber) {
    int sockfd;
    struct sockaddr_in serv_addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portnumber);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

Order generate_order(double p, double q, int numberOfMeals) {
    Order order;
    order.pid = getpid();
    order.id = 0;
    order.townWidth = p;
    order.townHeight = q;
    order.numberOfMeals = numberOfMeals;
    order.clientSocket = -1;
    order.meals = (Meal *)malloc(numberOfMeals * sizeof(Meal));

    srand(time(NULL)); // Seed with time to ensure different values
    for (int i = 0; i < numberOfMeals; i++) {
        order.meals[i].orderPid = order.pid;
        order.meals[i].mealId = i;
        order.meals[i].x = (double)(rand() % (int)(p * 1000)) / 1000;
        order.meals[i].y = (double)(rand() % (int)(q * 1000)) / 1000;
    }

    return order;
}

void send_order(int sockfd, Order *order) {
    // Send the order header first
    if (send(sockfd, order, sizeof(Order), 0) < 0) {
        perror("Failed to send order header");
        return;
    }

    // Send the meals array
    if (send(sockfd, order->meals, order->numberOfMeals * sizeof(Meal), 0) < 0) {
        perror("Failed to send meals array");
    }

    printOrder(order); // Print the order after sending
}

void wait_for_response(int sockfd) {
    char buffer[BUFFER_SIZE];
    while (1) {
        int bytes_received = recv(sockfd, buffer, BUFFER_SIZE, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            printf("Server: %s\n", buffer);
        } else {
            break;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s [portnumber] [numberOfMeals] [p] [q]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int portnumber = atoi(argv[1]);
    int numberOfMeals = atoi(argv[2]);
    double p = atof(argv[3]);
    double q = atof(argv[4]);

    int sockfd = connect_to_server(portnumber);

    Order order = generate_order(p, q, numberOfMeals);
    send_order(sockfd, &order);

    wait_for_response(sockfd);

    close(sockfd);
    free(order.meals);

    return 0;
}
