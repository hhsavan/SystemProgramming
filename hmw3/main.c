#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

#define MAX_AUTOMOBILES 8
#define MAX_PICKUPS 4

// Semaphore declarations
sem_t newPickup, inChargeforPickup, newAutomobile, inChargeforAutomobile;

// Counter variables to track available temporary parking spaces
int mFree_automobile = MAX_AUTOMOBILES;
int mFree_pickup = MAX_PICKUPS;

// Function prototypes
void *carOwner(void *arg);
void *carAttendant(void *arg);

int main() {
    // Initialize semaphores
    sem_init(&newPickup, 0, 0);
    sem_init(&inChargeforPickup, 0, 1);
    sem_init(&newAutomobile, 0, 0);
    sem_init(&inChargeforAutomobile, 0, 1);

    // Create threads for carOwner and carAttendant
    pthread_t ownerThread, attendantThread;
    pthread_create(&ownerThread, NULL, carOwner, NULL);
    pthread_create(&attendantThread, NULL, carAttendant, NULL);

    // Join threads
    pthread_join(ownerThread, NULL);
    pthread_join(attendantThread, NULL);

    // Destroy semaphores
    sem_destroy(&newPickup);
    sem_destroy(&inChargeforPickup);
    sem_destroy(&newAutomobile);
    sem_destroy(&inChargeforAutomobile);

    return 0;
}

// Car owner thread function
void *carOwner(void *arg) {
    while (1) {
        // Simulate arrival of a vehicle (1 for automobile, 2 for pickup)
        int vehicleType = rand() % 2 + 1;

        // Check if there is space available in the temporary parking lot
        if (vehicleType == 1 && mFree_automobile > 0) {
            sem_wait(&inChargeforAutomobile); // Lock access to mFree_automobile
            mFree_automobile--; // Decrement available automobile spaces
            sem_post(&newAutomobile); // Signal the attendant about the new automobile
            sem_post(&inChargeforAutomobile); // Release access to mFree_automobile
            printf("Owner: Automobile arrived\n");
        } else if (vehicleType == 2 && mFree_pickup > 0) {
            sem_wait(&inChargeforPickup); // Lock access to mFree_pickup
            mFree_pickup--; // Decrement available pickup spaces
            sem_post(&newPickup); // Signal the attendant about the new pickup
            sem_post(&inChargeforPickup); // Release access to mFree_pickup
            printf("Owner: Pickup arrived\n");
        } else {
            printf("Owner: No space available, exiting.\n");
        }

        // Simulate time between vehicle arrivals
        sleep(1);
    }
    return NULL;
}

// Car attendant thread function
void *carAttendant(void *arg) {
    while (1) {
        // Wait for a new automobile to arrive
        sem_wait(&newAutomobile);
        printf("Attendant: Automobile parked\n");
        // Simulate parking process
        sleep(2);
        // Increment available automobile spaces
        sem_wait(&inChargeforAutomobile); // Lock access to mFree_automobile
        mFree_automobile++;
        sem_post(&inChargeforAutomobile); // Release access to mFree_automobile

        // Wait for a new pickup to arrive
        sem_wait(&newPickup);
        printf("Attendant: Pickup parked\n");
        // Simulate parking process
        sleep(2);
        // Increment available pickup spaces
        sem_wait(&inChargeforPickup); // Lock access to mFree_pickup
        mFree_pickup++;
        sem_post(&inChargeforPickup); // Release access to mFree_pickup
    }
    return NULL;
}
