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

#define FIFO1 "fifo1"
#define FIFO2 "fifo2"
#define MAX_ELEMENTS 10

int main()
{

    // create fifos
    if (mkfifo(FIFO1, 0666) == -1 || mkfifo(FIFO2, 0666) == -1)
    {
        perror("mkfifo");
        exit(1);
    }

    // 2- Generate random numbers
    pid_t pid1, pid2;
    int sum2 = 0;
    srand(time(NULL));
    int arr[MAX_ELEMENTS];
    for (int i = 0; i < MAX_ELEMENTS; i++)
    {
        arr[i] = rand() % 10;
        ////printf"Generated number: %d\n", arr[i]);
        sum2 += arr[i];
    }

    // child processleri oluştur:

    //! CHILD1
    pid1 = fork();
    if (pid1 < 0)
    {
        perror("fork");
        exit(1);
    }
    else if (pid1 == 0)
    {
        printf("child1 sleeping...\n");
        sleep(5); // 10sn olmalı
        int sum = 0;
        int fd;

        // TODO 1- arr'i al
        if ((fd = open(FIFO1, O_RDONLY)) < 0)
        {
            perror("open");
            exit(1);
        }
        read(fd, arr, sizeof(arr));
        close(fd);

        // TODO 2- sum ı hesapla

        for (int i = 0; i < MAX_ELEMENTS; i++)
        {
            sum += arr[i];
            // printf"%d ", arr[i]);
        }

        // TODO 2- sum hesaplandı. Şimdi sum'ı child2ye gönder. child2nin bunu beklediğinden emin olunmalı
        //? Bunun yerine Sleep de kullanılabilir
        if ((fd = open(FIFO1, O_RDONLY)) < 0)
        {
            perror("open");
            exit(1);
        }
        char command[3];
        read(fd, command, sizeof(command));
        close(fd);

        if (strcmp("ok", command) == 0)
        { // main processten yetki geldi process2ye sum'ı gönderebilir
            if ((fd = open(FIFO2, O_WRONLY)) < 0)
            {
                perror("open");
                exit(1);
            }
            // printf"child1 sayıları yazmak için bekliyor\n");

            write(fd, &sum, sizeof(int));
            close(fd);
        }
        else
            printf("Main Processten Ok gelmedi! gelen: %s\n", command);

        printf("child1 End: %d", getpid());
        exit(0);
    }

    //! CHILD 2
    pid2 = fork();
    if (pid2 < 0)
    {
        perror("fork");
        exit(1);
    }
    else if (pid2 == 0)
    {
        printf("child2 sleeping...\n");
        sleep(5); // 10sn olmalı
        int sum = 0;
        int fd;

        // TODO 1- arr'i al
        if ((fd = open(FIFO2, O_RDONLY)) < 0)
        {
            perror("open");
            exit(1);
        }
        read(fd, arr, sizeof(arr) / sizeof(int));
        close(fd);

        // TODO 2- "multiply" komutunu al
        if ((fd = open(FIFO2, O_RDONLY)) < 0)
        {
            perror("open");
            exit(1);
        }
        char command[10];
        read(fd, command, sizeof(command));
        close(fd);
        // printf"child2 multiply comutunu aldı\n");

        printf("Alınan command: %s\n", command);

        // TODO Komutu kontrol et eğer "multiply" ise işlemi yap
        //  Check if command is "multiply"
        if (strcmp(command, "multiply") != 0)
        {
            printf("Error: Invalid command received.\n");
            exit(1);
        }

        // Calculate product
        int result = 1;
        printf("Alınan Arr: ");
        for (int i = 0; i < MAX_ELEMENTS; i++)
        {
            printf("%d ", arr[i]);
            result *= arr[i];
        }

        // TODO Şimdi sum'ı alabiliriz
        if ((fd = open(FIFO2, O_RDONLY)) < 0)
        {
            perror("open");
            exit(1);
        }
        int sumFromChild1 = 0;
        read(fd, &sumFromChild1, sizeof(int));
        close(fd);

        printf("\n----------\n");
        printf("Sum: %d\n", sumFromChild1);

        printf("Product: %d\n", result);

        printf("Result: %d\n", result + sumFromChild1);

        exit(0); // Exit with product as status
    }

    //! MAIN PROCESS
    int fd1 = open(FIFO1, O_WRONLY);
    int fd2 = open(FIFO2, O_WRONLY);
    if (fd1 < 0 || fd2 < 0)
    {
        perror("open");
        exit(1);
    }

    // TODO arr'i child1 ve child2ye gönder
    write(fd1, arr, sizeof(arr)); // child1 e gidecek
    write(fd2, arr, sizeof(arr)); // child2 e gidecek

    close(fd1);
    close(fd2);

    fd1 = open(FIFO1, O_WRONLY);
    fd2 = open(FIFO2, O_WRONLY);
    if (fd1 < 0 || fd2 < 0)
    {
        perror("open");
        exit(1);
    }
    // TODO child2ye multiply komutunu gönder
    write(fd2, "multiply", sizeof("multiply")); // child2 ye gidecek

    // TODO child1e ok komutunu gönder
    write(fd1, "ok", sizeof("ok")); // child1 e gidecek

    close(fd1);
    close(fd2);

    return 0;
}