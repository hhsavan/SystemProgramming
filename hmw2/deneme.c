#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>

#define FIFO1 "fifo1"
#define FIFO2 "fifo2"

int child_counter = 0;

void sigchld_handler(int signum, siginfo_t *info, void *context)
{
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        printf("Child process %d exited with status %d\n", pid, WEXITSTATUS(status));
        child_counter++;
    }
}

int main(int argc, char const *argv[])
{
    struct sigaction sa;

    sa.sa_sigaction = &sigchld_handler;
    sa.sa_flags = SA_SIGINFO;

    sigaction(SIGCHLD, &sa, NULL);

    srand(time(NULL));

    if (mkfifo(FIFO1, 0666) == -1 && errno != EEXIST)
    {
        perror("Error creating FIFO1");
        exit(EXIT_FAILURE);
    }
    if (mkfifo(FIFO2, 0666) == -1 && errno != EEXIST)
    {
        perror("Error creating FIFO1");
        exit(EXIT_FAILURE);
    }

    int number;
    printf("Enter an integer: ");
    scanf("%d", &number);

    pid_t child1, child2;
    child1 = fork();

    if (child1 == 0)
    {
        sleep(10);

        int fd1 = open(FIFO1, O_RDONLY);
        int sum = 0;
        int num;
        while (read(fd1, &num, sizeof(int)) > 0)
        {
            sum += num;
        }

        printf("sum = %d\n", sum);

        close(fd1);

        sleep(1);
        int fd2 = open(FIFO2, O_WRONLY);
        write(fd2, &sum, sizeof(int));
        close(fd2);

        exit(0);
    }
    else if ((child2 = fork()) == 0)
    {
        sleep(10);

        int fd2 = open(FIFO2, O_RDONLY);
        int mult = 1;
        int sum, i;
        char command[9];

        int *arr = malloc(sizeof(int) * number);

        read(fd2, arr, sizeof(int) * number);
        read(fd2, command, sizeof(command));
        close(fd2);

        if (strcmp(command, "multiply") == 0)
        {
            for (i = 0; i < number; i++)
            {
                mult *= arr[i];
            }
        }
        printf("mult = %d\n", mult);

        fd2 = open(FIFO2, O_RDONLY);

        read(fd2, &sum, sizeof(int));
        close(fd2);

        printf("result = %d\n", sum + mult);
        free(arr);
        exit(0);
    }
    else
    {
        int *arr = malloc(sizeof(int) * number);
        for (size_t i = 0; i < number; i++)
        {
            arr[i] = rand() % 10;
        }
        for (size_t i = 0; i < number; i++)
        {
            printf("%d ", arr[i]);
        }
        printf("\n");

        // Write integer to FIFOs
        int fd1 = open(FIFO1, O_WRONLY);
        if (fd1 == 0)
        {
            printf("fd1 0");
            exit(1);
        }
        if (write(fd1, arr, sizeof(int) * number) == -1)
        {
            printf("write failed!!");
            exit(-1);
        }
        if (close(fd1) == 1)
        {
            printf("file close succeed");
        }

        int fd2 = open(FIFO2, O_WRONLY);
        if (fd2 == 0)
        {
            printf("fd2 0");
            exit(1);
        }
        if (write(fd2, arr, sizeof(int) * number) == -1)
        {
            printf("write failed!!");
            exit(-1);
        }
        free(arr);

        char command[] = "multiply";
        if (write(fd2, command, sizeof(command)) == -1)
        {
            printf("write failed!!");
            exit(-1);
        }
        close(fd2);
        while (child_counter < 2)
        {
            printf("proceeding\n");
            sleep(2);
        }
    }

    return 0;
}
