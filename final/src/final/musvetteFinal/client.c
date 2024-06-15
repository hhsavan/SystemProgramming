#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>
#include <fcntl.h>

#define BUFFER_SIZE 1024

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

int sockfd;
int portnumber;
Order order;

// pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
char logFileName[256];
int logFile;

void logStatus(const char *status)
{
    // pthread_mutex_lock(&log_mutex);
    dprintf(logFile, "%s\n", status);
    // pthread_mutex_unlock(&log_mutex);
}

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
    case SERVERDOWN:
        return "SERVERDOWN";
    default:
        return "UNKNOWN";
    }
}

void printOrder(const Order *order)
{
    char logMsg[256];
    snprintf(logMsg, sizeof(logMsg), "Order ID: %d\n", order->id);
    logStatus(logMsg);

    printf("Order ID: %d\n", order->id);
    snprintf(logMsg, sizeof(logMsg), "Order ID: %d\n", order->id);
    logStatus(logMsg);
    printf("PID: %d\n", order->pid);
    snprintf(logMsg, sizeof(logMsg), "Order ID: %d\n", order->id);
    logStatus(logMsg);
    printf("Town Width: %.2f, Town Height: %.2f\n", order->townWidth, order->townHeight);
    snprintf(logMsg, sizeof(logMsg), "Order ID: %d\n", order->id);
    logStatus(logMsg);
    printf("Number of Meals: %d\n", order->numberOfMeals);
    snprintf(logMsg, sizeof(logMsg), "Order ID: %d\n", order->id);
    logStatus(logMsg);
    printf("Client Socket: %d\n", order->clientSocket);
    snprintf(logMsg, sizeof(logMsg), "Order ID: %d\n", order->id);
    logStatus(logMsg);
    for (int i = 0; i < order->numberOfMeals; i++)
    {
        printf("  Meal %d:\n", i + 1);
        snprintf(logMsg, sizeof(logMsg), "  Meal %d:", i + 1);
        logStatus(logMsg);
        printf("    Order PID: %d\n", order->meals[i].orderPid);
        snprintf(logMsg, sizeof(logMsg), "    Order PID: %d", order->meals[i].orderPid);
        logStatus(logMsg);

        printf("    Meal ID: %d\n", order->meals[i].mealId);
        snprintf(logMsg, sizeof(logMsg), "    Meal ID: %d", order->meals[i].mealId);
        logStatus(logMsg);

        printf("    Coordinates: (%.2f, %.2f)\n", order->meals[i].x, order->meals[i].y);
        snprintf(logMsg, sizeof(logMsg), "    Coordinates: (%.2f, %.2f)", order->meals[i].x, order->meals[i].y);
        logStatus(logMsg);

        printf("    Status: %s\n", getStatusString(order->meals[i].status));
        snprintf(logMsg, sizeof(logMsg), "    Status: %s", getStatusString(order->meals[i].status));
        logStatus(logMsg);
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
        logStatus("Failed to send meals array");
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

            char logMsg[256];
            snprintf(logMsg, sizeof(logMsg), "Meal %d for order %d updated to status %s", meal.mealId, meal.orderPid, getStatusString(meal.status));
            logStatus(logMsg);
            if (meal.status == DELIVERED)
            {
                deliveredCount++;
                if (deliveredCount == numberOfMeals)
                {
                    printf("All customers served\n");
                    logStatus("All customers served");
                    break;
                }
            }
            else if (meal.status == SERVERDOWN)
            {
                printf("PideShop is burned down. Server is down.\n");
                logStatus("PideShop is burned down. Server is down.");
                break;
            }
        }
        else
        {
            break;
        }
    }
}

void cleanup()
{
    close(sockfd);
    free(order.meals);
    close(logFile);
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
    printf("^C signal .. cancelling orders.. editing log..\n");
    logStatus("^C signal .. cancelling orders.. editing log..");
    free(cancelOrder.meals);

    cleanup();
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

    snprintf(logFileName, sizeof(logFileName), "Client_%d_log.txt", getpid());
    logFile = open(logFileName, O_CREAT | O_WRONLY | O_APPEND, 0777);
    if (logFile == -1)
    {
        perror("Failed to open log file");
        exit(EXIT_FAILURE);
    }

    sockfd = connect_to_server(portnumber);

    order = generate_order(p, q, numberOfMeals);
    send_order(sockfd, &order);

    receive_updates(sockfd, numberOfMeals);

    cleanup();

    return 0;
}
