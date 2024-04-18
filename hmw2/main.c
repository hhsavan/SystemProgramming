#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>

#define FIFO1 "fifo1"
#define FIFO2 "fifo2"
#define MAX_ELEMENTS 3
int child_count = 0; // Counter for child processes
pid_t pid1, pid2;

// Function to handle SIGCHLD signal
void sigchld_handler(int signo)
{
    pid_t pid;
    int status;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        printf("Child process %d exited with status: %d\n", pid, WEXITSTATUS(status));
        if (WEXITSTATUS(status) != 0)
        { // it means that a child exited with an error
            if(pid == pid1) pid = pid2;
            else pid = pid1;
            if (kill(pid, SIGTERM) == -1)
            {
                perror("kill");
                exit(EXIT_FAILURE);
            }
        }
        child_count++;
    }

    // if (child_count == 2)
    // {
    //     printf("All child processes have exited.\n");
    //     // exit(0);
    // }
}

void unlinkFifo(const char *fifo_name)
{

    if (unlink(fifo_name) == -1)
    {
        perror("Error unlinking FIFO");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char const *argv[])
{

    if (argc != 2)
    {
        printf("Usage: ./executable <RandomArrayLength>\n");
        return -1;
    }
    int ArrLength = atoi(argv[1]);

    if (mkfifo(FIFO1, 0666) == -1 && errno != EEXIST)
    {
        perror("mkfifo");
        exit(6);
    }
    // create fifos
    if (mkfifo(FIFO2, 0666) == -1 && errno != EEXIST)
    {
        perror("mkfifo");
        exit(6);
    }

    // 2- Generate random numbers
    int sum2 = 0;
    srand(time(NULL));

    // int arr[MAX_ELEMENTS];
    int *arr = (int *)malloc(sizeof(int) * ArrLength);

    printf("Array: ");
    for (int i = 0; i < ArrLength; i++)
    {
        arr[i] = rand() % 10;
        printf("%d ", arr[i]);
        ////printf"Generated number: %d\n", arr[i]);
        sum2 += arr[i];
    }
    printf("\n");

    // child processleri oluştur:

    //! CHILD1
    pid1 = fork();
    if (pid1 < 0)
    {
        perror("fork");
        exit(5);
    }
    else if (pid1 == 0)
    {

        int sum = 0;
        int fd;
        // int arr2[MAX_ELEMENTS];
        int *arr2 = (int *)malloc(sizeof(int) * ArrLength);

        // TODO 1- arr'i al
        if ((fd = open(FIFO1, O_RDONLY)) < 0)
        {
            perror("open");
            exit(1);
        }
        // printf("child1 sleeping...\n");
        sleep(10); // 10sn olmalı

        if (-1 == read(fd, arr2, sizeof(int) * ArrLength))
        {
            perror("read");
            exit(1);
        }
        // close(fd);
        // printf"child2 multiply comutunu aldı\n");
        if (close(fd) == -1)
        {
            perror("close");
            exit(4);
        }
        // TODO 2- sum ı hesapla

        for (int i = 0; i < ArrLength; i++)
        {
            sum += arr2[i];
            // printf"%d ", arr[i]);
        }

        // TODO child2ye sum'ı gönder
        if ((fd = open(FIFO2, O_WRONLY)) < 0)
        {
            perror("open");
            exit(1);
        }
        sleep(1);
        if (-1 == write(fd, &sum, sizeof(int)))
        {
            perror("write");
            exit(3);
        }

        if (close(fd) == -1)
        {
            perror("close");
            exit(4);
        }
        free(arr2);
        // printf("child1 End: %d\n", getpid());
        exit(0);
    }

    //! CHILD 2
    pid2 = fork();
    if (pid2 < 0)
    {
        perror("fork");
        exit(5);
    }
    else if (pid2 == 0)
    {
        int sum = 0;
        int fd;
        // char command[10];
        // int arr2[MAX_ELEMENTS];
        int *arr2 = (int *)malloc(sizeof(int) * ArrLength);
        char *command = (char *)malloc(sizeof(char) * ArrLength);

        // TODO 1- arr'i al
        if ((fd = open(FIFO2, O_RDONLY)) < 0)
        {
            perror("open");
            exit(1);
        }
        // printf("child2 sleeping...\n");
        sleep(10);
        close(fd);
        if (-1 == read(fd, arr2, sizeof(int) * ArrLength))
        {
            perror("read");
            exit(1);
        }
        // // TODO 2- "multiply" komutunu al
        // if ((fd = open(FIFO2, O_RDONLY)) < 0)
        // {
        //     perror("open");
        //     exit(1);
        // }
        if (-1 == read(fd, command, sizeof(char) * sizeof("multiply")))
        {
            perror("read");
            exit(1);
        }
        // close(fd);
        // printf"child2 multiply comutunu aldı\n");
        if (close(fd) == -1)
        {
            perror("close");
            exit(4);
        }

        // printf("Alınan command: %s\n", command);

        // TODO Komutu kontrol et eğer "multiply" ise işlemi yap
        //  Check if command is "multiply"
        if (strcmp(command, "multiply") != 0)
        {
            printf("Error: Invalid command received.\n");
            exit(5);
        }

        // Calculate product
        int result = 1;
        for (int i = 0; i < ArrLength; i++)
            result *= arr[i];

        // TODO Şimdi sum'ı alabiliriz
        if ((fd = open(FIFO2, O_RDONLY)) < 0)
        {
            perror("open");
            exit(2);
        }
        int sumFromChild1 = -1;
        if (-1 == read(fd, &sumFromChild1, sizeof(int)))
        {
            perror("read");
            exit(1);
        }
        if (close(fd) == -1)
        {
            perror("close");
            exit(4);
        }

        printf("Sum: %d\n", sumFromChild1);
        printf("Product: %d\n", result);
        printf("Result (sum + product): %d\n", result + sumFromChild1);
        free(arr2);
        free(command);

        // printf("child2 End: %d\n", getpid());
        exit(0); // Exit with product as status
    }

    //! MAIN PROCESS
    int fd1 = open(FIFO1, O_WRONLY);
    int fd2 = open(FIFO2, O_WRONLY);
    if (fd1 < 0 || fd2 < 0)
    {
        perror("open");
        exit(2);
    }
    // TODO arr'i child1 ve child2ye gönder
    if (-1 == write(fd1, arr, sizeof(int) * ArrLength))
    {
        perror("write");
        exit(3);
    } // child1 e gidecek
    if (-1 == write(fd2, arr, sizeof(int) * ArrLength))
    {
        perror("write");
        exit(3);
    } // child1 e gidecek
      // TODO child2ye multiply komutunu gönder
    if (-1 == write(fd2, "multiply", sizeof("multiply")))
    {
        perror("write");
        exit(3);
    } // child1 e gidecek

    if (close(fd1) == -1)
    {
        perror("close");
        exit(4);
    }
    if (close(fd2) == -1)
    {
        perror("close");
        exit(4);
    }

    // Set signal handler for SIGCHLD
    signal(SIGCHLD, sigchld_handler);

    // Loop for printing "proceeding" message
    while (child_count < 2)
    {
        sleep(2); // Sleep for 2 seconds
        printf("proceeding...\n");
        // fflush(stdout);
    }
    unlinkFifo(FIFO1);
    unlinkFifo(FIFO2);
    free(arr);
    return 0;
}