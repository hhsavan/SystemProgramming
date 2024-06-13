#include "helper.h"

void calculate_pseudo_inverse(double complex matrix[30][40], double complex result[40][30]) {
    // Placeholder for actual pseudo-inverse calculation
    // You need to replace this with the actual implementation
    for (int i = 0; i < 40; i++) {
        for (int j = 0; j < 30; j++) {
            result[i][j] = 1.0 / (1.0 + matrix[j][i]);
        }
    }
}

// Function to log order status
void log_order_status(Order *order) {
    printf("Order %d status: %d\n", order->id, order->status);
}
