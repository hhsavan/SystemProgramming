#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>

#define BUFFER_SIZE 1024

typedef enum
{
    PLACED,
    PREPARED,
    COOKED,
    IN_DELIVERY,
    DELIVERED,
    CANCELED
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

int sockfd;
int portnumber;
Order order;

const char *getStatusString(MealStatus status)
{
    switch (status)
    {
    case PLACED:
        return "PLACED";
    case PREPARED:
        return "PREPARED";
    case COOKED:
        return "COOKED";
    case IN_DELIVERY:
        return "IN_DELIVERY";
    case DELIVERED:
        return "DELIVERED";
    case CANCELED:
        return "CANCELED";
    default:
        return "UNKNOWN";
    }
}

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
        printf("    Status: %s\n", getStatusString(order->meals[i].status));
    }
}

int connect_to_server(int portnumber)
{
    int sockfd;
    struct sockaddr_in serv_addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portnumber);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

Order generate_order(double p, double q, int numberOfMeals)
{
    Order order;
    order.pid = getpid();
    order.id = 0;
    order.townWidth = p;
    order.townHeight = q;
    order.numberOfMeals = numberOfMeals;
    order.clientSocket = -1;
    order.meals = (Meal *)malloc(numberOfMeals * sizeof(Meal));

    srand(time(NULL)); // Seed with time to ensure different values
    for (int i = 0; i < numberOfMeals; i++)
    {
        order.meals[i].orderPid = order.pid;
        order.meals[i].mealId = i;
        order.meals[i].x = (double)(rand() % (int)(p * 1000)) / 1000;
        order.meals[i].y = (double)(rand() % (int)(q * 1000)) / 1000;
        order.meals[i].status = PLACED;
    }

    return order;
}

void send_order(int sockfd, Order *order)
{
    // Send the order header first
    if (send(sockfd, order, sizeof(Order), 0) < 0)
    {
        perror("Failed to send order header");
        return;
    }

    // Send the meals array
    if (send(sockfd, order->meals, order->numberOfMeals * sizeof(Meal), 0) < 0)
    {
        perror("Failed to send meals array");
    }

    printOrder(order); // Print the order after sending
}

void receive_updates(int sockfd, int numberOfMeals)
{
    Meal meal;
    int deliveredCount = 0;
    while (1)
    {
        int bytes_received = recv(sockfd, &meal, sizeof(Meal), 0);
        if (bytes_received > 0)
        {
            printf("Meal %d for order %d updated to status %s\n", meal.mealId, meal.orderPid, getStatusString(meal.status));
            if (meal.status == DELIVERED)
            {
                deliveredCount++;
                if (deliveredCount == numberOfMeals)
                {
                    printf("All customers served\n");
                    break;
                }
            }
        }
        else
        {
            break;
        }
    }
}

void handle_signal(int sig)
{
    Order cancelOrder;
    cancelOrder.pid = order.pid;
    cancelOrder.id = order.id;
    cancelOrder.townWidth = order.townWidth;
    cancelOrder.townHeight = order.townHeight;
    cancelOrder.numberOfMeals = 1;
    cancelOrder.clientSocket = order.clientSocket;
    cancelOrder.meals = (Meal *)malloc(sizeof(Meal));
    cancelOrder.meals[0].orderPid = order.pid;
    cancelOrder.meals[0].mealId = -1;
    cancelOrder.meals[0].x = 0;
    cancelOrder.meals[0].y = 0;
    cancelOrder.meals[0].status = CANCELED;
    cancelOrder.meals[0].clientSocket = sockfd;
    int tempSockfd = connect_to_server(portnumber);
    send_order(tempSockfd, &cancelOrder);
    free(cancelOrder.meals);

    exit(0);
}

int main(int argc, char *argv[])
{
    if (argc != 5)
    {
        fprintf(stderr, "Usage: %s [portnumber] [numberOfMeals] [p] [q]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, handle_signal);

    portnumber = atoi(argv[1]);
    int numberOfMeals = atoi(argv[2]);
    double p = atof(argv[3]);
    double q = atof(argv[4]);

    sockfd = connect_to_server(portnumber);

    order = generate_order(p, q, numberOfMeals);
    send_order(sockfd, &order);

    receive_updates(sockfd, numberOfMeals);

    close(sockfd);
    free(order.meals);

    return 0;
}
