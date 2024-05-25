#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <semaphore.h>
#include <unistd.h>
#include <errno.h>

#define COMING_CAR_NUM 15
// #define COMING_Pickup_NUM 20
#define TEMP_AUTOMOBILE_SPACES 8 // bekleme yeri
#define TEMP_PICKUP_SPACES 4     // bekleme yeri

#define AUTOMOBILE_VALE 1
#define PICKUP_VALE 0
#define AUTOMOBILE 1
#define PICKUP 0

int mFree_automobile = TEMP_AUTOMOBILE_SPACES;
int mFree_pickup = TEMP_PICKUP_SPACES;
// critical variables

// sem_t newPickup, inChargeforPickup;         // Semaphores for pickups
// sem_t newAutomobile, inChargeforAutomobile; // Semaphores for automobiles
sem_t newPickup, inChargeforPickup;         // Semaphores for pickups
sem_t newAutomobile, inChargeforAutomobile; // Semaphores for automobiles

sem_t sem_mFree;
size_t currentNum = 0;
// sıfır veya 1 dönderirç
//  0: pickup, 1:automobile
int car()
{
    return rand() % 2;
}
void *carOwner(void *num)
{
    //
    // sleep(rand() % 3);
    int ownerType = *((int *)num);

    if (ownerType == AUTOMOBILE)
    {
        while (1)
        {

            // sem_wait(&newAutomobile);

            if (sem_trywait(&newAutomobile) == -1)
            {
                if (currentNum == COMING_CAR_NUM)
                    break;
                continue;
            }
            sem_wait(&sem_mFree);

            if (mFree_automobile == 0)
            { // Check if automobile space available
                printf("Owner(Automobile): No temporary parking space for my automobile, leaving...\n");
                fflush(stdout);
                sem_post(&sem_mFree);
                // sem_post(&newAutomobile);
                // return NULL;
                continue;
            }
            mFree_automobile--;
            sem_post(&sem_mFree);

            printf("Owner(Automobile): My automobile is here. Take care of my car...\n");
            fflush(stdout);
            sem_post(&inChargeforAutomobile);
        }
    }
    else if (ownerType == PICKUP)
    {
        while (1)
        {
            // sem_wait(&newPickup);
            if (sem_trywait(&newPickup) == -1)
            {
                if (currentNum == COMING_CAR_NUM)
                    break;
                continue;
            }
            sem_wait(&sem_mFree);

            if (mFree_pickup == 0)
            { // Check if Pickup space available
                printf("Owner(Pickup): No temporary parking space for my Pickup, leaving...\n");
                fflush(stdout);
                sem_post(&sem_mFree);
                // sem_post(&newPickup);
                // return NULL;
                // continue;
            }
            else
            {
                mFree_pickup--;
                sem_post(&sem_mFree);

                printf("Owner(Pickup): My Pickup is here. Take care of my car...\n");
                fflush(stdout);
                sem_post(&inChargeforPickup);
            }
        }
    }
    else
    {
        perror("Unknown owner/vehicle type!\n");
        pthread_exit(NULL);
    }
    // return NULL;
    pthread_exit(NULL);
}

void *carAttendent(void *type)
{
    int vale = *((int *)type);
    int l_trycount = 0;
    while (1)
    {
        if (vale == AUTOMOBILE_VALE)
        {
            // Wait for an automobile to park
            // sem_wait(&inChargeforAutomobile);

            if (sem_trywait(&inChargeforAutomobile) == -1)
            {
                if (currentNum == COMING_CAR_NUM && mFree_automobile == TEMP_AUTOMOBILE_SPACES)
                    break;
                continue;
            }

            sem_wait(&sem_mFree);
            mFree_automobile++;
            printf("Valet(Automobile): An automobile parked now\n");
            fflush(stdout);
            sem_post(&sem_mFree);
            sleep(3);
            // printf("# An automobile is parked\n");
            // fflush(stdout);
        }
        else if (vale == PICKUP_VALE)
        {

            // Wait for an automobile to park
            // sem_wait(&inChargeforPickup);
            if (sem_trywait(&inChargeforPickup) == -1)
            {
                if (currentNum == COMING_CAR_NUM && mFree_pickup == TEMP_PICKUP_SPACES)
                    break;
                continue;
            }
            sem_wait(&sem_mFree);
            mFree_pickup++;
            printf("Valet(pickup): A pickup parked now\n");
            fflush(stdout);
            sem_post(&sem_mFree);
            sleep(3);
            // printf("# A pickup is parked.\n");
            // fflush(stdout);
        }
        else
        {
            perror("Error Vale Type\n");
            // return NULL;
            pthread_exit(NULL);
        }
    }
    pthread_exit(NULL);

    // return NULL;
}

int main()
{
    srand(time(NULL));

    pthread_t vale1, vale2, carOwner1, carOwner2;
    int valeA = AUTOMOBILE_VALE;
    int valeP = PICKUP_VALE;

#pragma region sem_inits
    sem_init(&newPickup, 0, 0);
    sem_init(&inChargeforPickup, 0, 0);
    sem_init(&newAutomobile, 0, 0);
    sem_init(&inChargeforAutomobile, 0, 0); // 0-1 mutex gibi çalışır
    sem_init(&sem_mFree, 0, 1);
#pragma endregion
#pragma region creates
    if (pthread_create(&vale1, NULL, carAttendent, &valeA) != 0)
    {
        perror("pthread_create-ValeAutomobile");
        exit(1);
    }

    if (pthread_create(&vale2, NULL, carAttendent, &valeP) != 0)
    {
        perror("pthread_create-ValePickup");
        exit(1);
    }

    if (pthread_create(&carOwner1, NULL, carOwner, &valeA) != 0)
    {
        perror("pthread_create-Car");
        exit(1);
    }
    if (pthread_create(&carOwner2, NULL, carOwner, &valeP) != 0)
    {
        perror("pthread_create-Car");
        exit(1);
    }

    for (currentNum = 0; currentNum < COMING_CAR_NUM; currentNum++)
    {
        int carType = car();
        // carType = AUTOMOBILE;
        if (carType == AUTOMOBILE)
        {
            sem_post(&newAutomobile);
        }
        else
        {
            sem_post(&newPickup);
        }
        sleep(1);
    }
#pragma endregion
#pragma region joins
    if (pthread_join(vale1, NULL) != 0)
    {
        perror("pthread_join_vale1");
        exit(1);
    }

    if (pthread_join(vale2, NULL) != 0)
    {
        perror("pthread_join_vale1");
        exit(1);
    }

    if (pthread_join(carOwner1, NULL) != 0)
    {
        perror("pthread_join_owner1");
        exit(1);
    }
    if (pthread_join(carOwner2, NULL) != 0)
    {
        perror("pthread_join_owner2");
        exit(1);
    }
#pragma endregion

    sem_destroy(&newPickup);
    sem_destroy(&inChargeforPickup);
    sem_destroy(&newAutomobile);
    sem_destroy(&inChargeforAutomobile);
    sem_destroy(&sem_mFree);

    return 0;
}
