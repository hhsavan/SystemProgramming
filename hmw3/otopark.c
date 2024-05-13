#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <semaphore.h>
#include <unistd.h>
#include <errno.h>

#define COMING_CAR_NUM 50
#define MAX_AUTOMOBILES 8
#define MAX_PICKUPS 4
#define TEMP_AUTOMOBILE_SPACES 8 // bekleme yeri
#define TEMP_PICKUP_SPACES 4     // bekleme yeri

#define AUTOMOBILE_VALE 1
#define PICKUP_VALE 0
#define AUTOMOBILE 1
#define PICKUP 0
int mFree_automobile = 8;
int mFree_pickup = 4;
// critical variables
int reaminingAutoSpaceInside = 8;
int reaminingPickupSpaceInside = 4;

int reaminingAutoSpaceOutside = 8;
int reaminingPickupSpaceOutside = 4;

// sem_t newPickup, inChargeforPickup;         // Semaphores for pickups
// sem_t newAutomobile, inChargeforAutomobile; // Semaphores for automobiles
sem_t pickupsayisi, inChargeforPickup;    // Semaphores for pickups
sem_t arabaSayisi, inChargeforAutomobile; // Semaphores for automobiles

sem_t sem_mFree_automobile, sem_mFree_pickup;

// sıfır veya 1 dönderirç
//  0: pickup, 1:automobile
int car()
{
    return rand() % 2;
}
void *carOwner()
{
    //
    // sleep(rand() % 3);

    int carType = car();

    if (carType == AUTOMOBILE)
    {

        // buraya da sem lazım mı?
        sem_wait(&sem_mFree_automobile);
        if (mFree_automobile == 0)
        { // Check if automobile space available
            printf("Car owner(Automobile): No temporary parking space for my automobile, leaving...\n");
            fflush(stdout);
            sem_post(&sem_mFree_automobile);
            return NULL;
        }
        mFree_automobile--;
        sem_post(&sem_mFree_automobile);
        // printf("Car owner(Automobile): Now I am waiting in temporary parking lot...\n");
        // fflush(stdout);
        // içeride yer var temp
        // sem_post(&sem_mFree_automobile);

        // valenin bunu almasını bekliyor. vale her seferinde bir tane alır. hangisini alırsa artık
        sem_post(&inChargeforAutomobile);
        printf("Car owner(Automobile): My automobile is being parked. Take care of my car...\n");
        fflush(stdout);
        // sem_post(&inChargeforAutomobile);
    }
    else if (carType == PICKUP)
    {
        // buraya da sem lazım mı?
        // sem_wait(&sem_mFree_pickup);
        sem_wait(&sem_mFree_pickup);
        if (mFree_pickup == 0)
        { // Check if automobile space available
            printf("Car owner(pickup): No temporary parking space for my pickup, leaving...\n");
            fflush(stdout);
            sem_post(&sem_mFree_pickup);
            return NULL;
        }
        mFree_pickup--;
        sem_post(&sem_mFree_pickup);
        // printf("Car owner(pickup): Now I am waiting in temporary parking lot...\n");
        // fflush(stdout);
        // içeride yer var temp
        // yeni arabba geldiği bildirilir:

        // valenin bunu almasını bekliyor. vale her seferinde bir tane alır. hangisini alırsa artık
        sem_post(&inChargeforPickup);
        printf("Car owner(pickup): My pickup is being parked. Take care of my car...\n");
        fflush(stdout);
    }
    else
    {
        perror("Error Car Type\n");
        return NULL;
    }

    return NULL;
}

void *carAttendent(void *type)
{
    int vale = *((int *)type);

    while (1)
    {
        if (vale == AUTOMOBILE_VALE)
        {
            // Wait for an automobile to park
            sem_wait(&inChargeforAutomobile);
            // buraya birden fazla thread yıpılıyor mu? yığılmaması lazım
            if (sem_wait(&arabaSayisi) == -1)
            {
                printf("Car Valet(Automobile): No parking space inside!\n");
                fflush(stdout);
            }
            else
            {
                sem_wait(&sem_mFree_automobile);
                mFree_automobile++;
                sem_post(&sem_mFree_automobile);
                printf("Car Valet(Automobile): I will park a car now\n");
                fflush(stdout);
                sleep(1);
                printf("Car Valet(Automobile): An automobile is parked\n");
                fflush(stdout);
            }
        }
        else if (vale == PICKUP_VALE)
        {

            // Wait for an automobile to park
            sem_wait(&inChargeforPickup);

            if (sem_wait(&pickupsayisi) == -1)
            {
                printf("Car Valet(pickup): No parking space inside!\n");
                fflush(stdout);
            }
            else
            {

                sem_wait(&sem_mFree_pickup);
                mFree_pickup++;
                sem_post(&sem_mFree_pickup);
                printf("Car Valet(pickup): I will park this car now\n");
                fflush(stdout);
                sleep(1);
                printf("Car Valet(pickup): An pickup is parked.\n");
                fflush(stdout);
            }
        }
        else
        {
            perror("Error Vale Type\n");
            return NULL;
        }
    }

    return NULL;
}

int main()
{
    srand(time(NULL)); // Seed random number generator

    pthread_t vale1;
    pthread_t vale2;
    pthread_t cars[COMING_CAR_NUM];
    int valeA = AUTOMOBILE_VALE;
    int valeP = PICKUP_VALE;

    sem_init(&pickupsayisi, 0, TEMP_PICKUP_SPACES);
    sem_init(&inChargeforPickup, 0, 0);
    sem_init(&arabaSayisi, 0, TEMP_AUTOMOBILE_SPACES);
    sem_init(&inChargeforAutomobile, 0, 0);

    sem_init(&sem_mFree_automobile, 0, 1);
    sem_init(&sem_mFree_pickup, 0, 1);

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

    for (int i = 0; i < COMING_CAR_NUM; i++)
    {
        if (pthread_create(&cars[i], NULL, carOwner, NULL) != 0)
        {
            perror("pthread_create-Car");
            exit(1);
        }
    }

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

    for (int i = 0; i < COMING_CAR_NUM; i++)
    {
        if (pthread_join(cars[i], NULL) != 0)
        {
            perror("pthread_join-Car");
            exit(1);
        }
    }
    return 0;
}
