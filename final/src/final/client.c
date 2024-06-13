#include "helper.h"
#include <stdlib.h>
#include <time.h>

// Function declarations
void place_order(int socket, int client_id, int num_clients, int p, int q);

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <portnumber> <numberOfClients> <p> <q>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int portnumber = atoi(argv[1]);
    int numberOfClients = atoi(argv[2]);
    int p = atoi(argv[3]);
    int q = atoi(argv[4]);

    srand(time(NULL));

    int sock = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket creation error\n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portnumber);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("Invalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Connection Failed \n");
        return -1;
    }

    place_order(sock, getpid(), numberOfClients, p, q);

    close(sock);

    return 0;
}

void place_order(int socket, int client_id, int num_clients, int p, int q) {
    Order new_order;
    new_order.pid = client_id;
    new_order.id = client_id;
    new_order.townWidth = p;
    new_order.townHeight = q;
    new_order.numberOfClients = num_clients;
    new_order.status = ORDER_PLACED;

    // Allocate and populate meals
    new_order.meals = malloc(num_clients * sizeof(Meal));
    for (int i = 0; i < num_clients; i++) {
        new_order.meals[i].orderPid = client_id;
        new_order.meals[i].clientId = i;
        new_order.meals[i].x = rand() % p;
        new_order.meals[i].y = rand() % q;
        new_order.meals[i].status = ORDER_PLACED;
        printf("Client %d placing meal %d at (%f, %f)\n", client_id, i, new_order.meals[i].x, new_order.meals[i].y);
    }

    send(socket, &new_order, sizeof(Order), 0);
    send(socket, new_order.meals, num_clients * sizeof(Meal), 0);

    printf("Client %d placed order with %d meals\n", client_id, num_clients);

    free(new_order.meals);
}
