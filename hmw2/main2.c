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

int child_count = 0; // Counter for child processes

// Function to handle SIGCHLD signal
void sigchld_handler(int signo)
{
    pid_t pid;
    int status;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
       //printf"Child process %d exited with status: %d\n", pid, WEXITSTATUS(status));
        child_count++;
    }

    if (child_count == 2)
    {
       //printf"All child processes have exited.\n");
        exit(0);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
       //printf"Usage: %s <integer>\n", argv[0]);
        exit(1);
    }

    int integer = atoi(argv[1]);

    // 1- Create FIFOs
    if (mkfifo(FIFO1, 0666) == -1 || mkfifo(FIFO2, 0666) == -1)
    {
        perror("mkfifo");
        exit(1);
    }

    pid_t pid1, pid2;
    int sum2 = 0;

    // 2- Generate random numbers
    srand(time(NULL));
    int arr[MAX_ELEMENTS];
    for (int i = 0; i < MAX_ELEMENTS; i++)
    {
        arr[i] = rand() % 10;
        ////printf"Generated number: %d\n", arr[i]);
        sum2 += arr[i];
    }

   //printf"1-sum: %d\n", sum2);

    // Create child processes
    pid1 = fork();
    if (pid1 < 0)
    {
        perror("fork");
        exit(1);
    }
    else if (pid1 == 0)
    { // Child process 1
    ;
        int sum = 0;
        int fd;
        // 4- child1 ye arr'i ver
        //  Open FIFO1 for reading
        if ((fd = open(FIFO1, O_RDONLY)) < 0)
        {
            perror("open");
            exit(1);
        }

       //printf"child1 sayıları bekliyor\n");

        read(fd, arr, sizeof(arr));
        close(fd);
       //printf"child1 sayıları aldı\n");

        // Calculate sum
        for (int i = 0; i < MAX_ELEMENTS; i++)
        {
            sum += arr[i];
           //printf"%d ", arr[i]);
        }
       //printf"\nchild1 sayıları topladı\n");

        // sleep(1);
        // main processden bir command bekliyoruz. böylelikle child2ye arrayin ulaştığına emin oluyoruz
        if ((fd = open(FIFO1, O_RDONLY)) < 0)
        {
            perror("open");
            exit(1);
        }
       //printf"child2 multiply comutunu bekliyor\n");

        char command[10];
        read(fd, command, sizeof(command));
        close(fd);
        // if(strcmp(command,"ok")==0){
        //     // child2 arrayi aldı

        // }

       //printf"child2 multiply comutunu aldı\n");
        // 7- sum'ı child1den child2ye gönder
        // Write sum to FIFO2
        if ((fd = open(FIFO2, O_WRONLY)) < 0)
        {
            perror("open");
            exit(1);
        }
       //printf"child1 sayıları yazmak için bekliyor\n");

        write(fd, &sum, sizeof(int));
        close(fd);
       //printf"child1 sayıları yazdı\n");

        exit(sum); // Exit with sum as status
    }

    //CHILD2
    pid2 = fork();
    if (pid2 < 0)
    {
        perror("fork");
        exit(1);
    }
    else if (pid2 == 0)
    { // Child process 2
    

        int result;
        int fd;

        // 4- child2 ye arr'i ver
        //  Open FIFO2 for reading (again)
        if ((fd = open(FIFO2, O_RDONLY)) < 0)
        {
            perror("open");
            exit(1);
        }
       //printf"child2 sayıları bekliyor\n");

        read(fd, arr, sizeof(arr));
        close(fd);
       //printf"child2 sayıları aldı\n");

        ///----------------------
        // 6- multiply komutunu al
        // Open FIFO2 for reading
        if ((fd = open(FIFO2, O_RDONLY)) < 0)
        {
            perror("open");
            exit(1);
        }
       //printf"child2 multiply comutunu bekliyor\n");

        char command[10];
        read(fd, command, sizeof(command));
        close(fd);
       //printf"child2 multiply comutunu aldı\n");

       printf("command: %s\n", command);

        // Check if command is "multiply"
        if (strcmp(command, "multiply") != 0)
        {
           printf("Error: Invalid command received.\n");
            exit(1);
        }

        // Calculate product
        result = 1;
        for (int i = 0; i < MAX_ELEMENTS; i++)
        {
            result *= arr[i];
        }
       //printf"Product: %d\n", result);

        //-------------

        // 8- child1den sum'ı ala
        if ((fd = open(FIFO2, O_RDONLY)) < 0)
        {
            perror("open");
            exit(1);
        }
       //printf"child2 sumı bekliyor\n");

        int sumFromchild1 = 0;
        read(fd, &sumFromchild1, sizeof(int));
        close(fd);
       //printf"child2 sum'ı aldı\n");

       //printf"----------\n");
       printf("Sum: %d\n", sumFromchild1);

       printf("Producvt: %d\n", result);

       printf("Result: %d\n", result + sumFromchild1);

        exit(result); // Exit with product as status
    }

    // Parent process
    // Open FIFOs for writing
   //printf"main process fifo1 i açtı\n");

    int fd1 = open(FIFO1, O_WRONLY);
   //printf"main process fifo2 i açtı\n");

    int fd2 = open(FIFO2, O_WRONLY);
    if (fd1 < 0 || fd2 < 0)
    {
        perror("open");
        exit(1);
    }

    // 3- arr'i 1 ve 2. childlara gönder
   //printf"main process arrayi child1 e gönderiyor\n");

    write(fd1, arr, sizeof(arr)); // child1 e gidecek
   //printf"main process arrayi child1 e gönderdi\n");

   //printf"main process arrayi child2 ye gönderiyor\n");

    write(fd2, arr, sizeof(arr)); // child2 e gidecek
   //printf"main process arrayi child2 ye gönderdi\n");

    // 5- multiply komutunu child2 ye gönder
   //printf"main process multiply komutunu gönderiyor\n");

    write(fd2, "multiply", sizeof("multiply")); // child2 ye gidecek
    write(fd1, "ok", sizeof("ok")); // child2 ye gidecek
   //printf"main process multiply komutunu gönderdi\n");

    close(fd1);
    close(fd2);

    // Set signal handler for SIGCHLD
    signal(SIGCHLD, sigchld_handler);

    // Loop for printing "proceeding" message
    while (child_count < 2)
    {
        sleep(2);// Sleep for 2 seconds
       //printf"proceeding...\n");
    }

    return 0;
}
