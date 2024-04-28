#include "src/helper.h"

// Global variables for signal handling
volatile sig_atomic_t received_signal = 0;
pid_t child_pid;

// Signal handler function
void signal_handler(int signum)
{
    received_signal = signum;
    if (signum == SIGINT)
    {
        printf("Received SIGINT signal. Terminating...\n");
    }
}

// Function to handle client connections
int handle_client(int client_num)
{
    printf("handle client\n");
    char client_pid_str[MAX_BUF_SIZE];
    snprintf(client_pid_str, sizeof(client_pid_str), "%d", getpid());
    char client_name[MAX_BUF_SIZE] = "client";
    strcat(client_name, client_pid_str);
    // printf(">> Client PID %d connected as \"%s\"\n", getpid(), client_name);

    int fd;
    if ((fd = open(clientFifo, O_WRONLY)) < 0)
    {
        perror("serverFifo open");
        exit(1);
    }

    ConnectionReq cReq;
    cReq.pid = getpid();
    cReq.type = established;
    printf("Bura1\n");
    if (-1 == write(fd, &cReq, sizeof(cReq)))
    {
        perror("write");
        return -1;
    }
    printf("Bura2\n");

    close(fd);

    if ((fd = open(clientFifo, O_RDONLY)) < 0)
    {
        perror("clientFifo open");
        exit(1);
    }
    printf("Bura3\n");

    Message msg;

    if (-1 == read(fd, &msg, sizeof(msg)))
    {
        perror("read");
        return -1;
    }
    printf("Bura4\n");

    // close(fd);

    // Simulating client-server interaction
    // Here you can implement actual communication with clients
    sleep(2); // Simulating some activity
    printf(">> %s disconnected..\n", client_name);
    exit(0); // Exit child process after handling client
}

int main(int argc, char *argv[])
{
    // if (argc != 3)
    // {
    //     printf("Usage: %s <dirname> <max. #ofClients>\n", argv[0]);
    //     exit(EXIT_FAILURE);
    // }

    // // Register signal handler for SIGINT (Ctrl-C)
    // signal(SIGINT, signal_handler);

    // // Create the specified directory if it doesn't exist
    // char *dirname = argv[1];
    // if (mkdir(dirname, 0777) == -1 && errno != EEXIST)
    // {
    //     perror("mkdir");
    //     exit(EXIT_FAILURE);
    // }
    // // change the working directory of the process
    // if (chdir(dirname) == -1)
    // {
    //     perror("chdir");
    //     exit(EXIT_FAILURE);
    // }

    // create fifos
    if (mkfifo(SERVER_FIFO, 0666) == -1 && errno != EEXIST)
    {
        // printf("Error while creating the fifo2\n");
        perror("mkfifo");
        exit(6);
    }
    // open fifo in rdwr mode
    int fd;
    // printf("fd: %d",fd);
    // log file add: server fifo opened

    // Set maximum number of clients
    // printf(">> Waiting for clients...\n");
    int max_clients = 3; // atoi(argv[2]);
    int current_clients = 0;
    struct ConnectionReq cReq;
    printf(">> Server Started PID %d...\n", getpid());
    printf(">> Waiting for clients...\n");
    if ((fd = open(SERVER_FIFO, O_RDONLY)) < 0)
    {
        perror("serverFifo open1");
        exit(1);
    }
    while (1)
    {
        // Check if a termination signal has been received
        if (received_signal == SIGINT)
        {
            printf(">> Exiting due to SIGINT signal...\n");
            // Send kill signal to child processes
            kill(child_pid, SIGTERM);
            // Close log file
            // close(log_fd);
            exit(EXIT_SUCCESS);
        }

        // Check if maximum number of clients has been reached
        if (current_clients >= max_clients)
        {
            printf(">> Connection request Que FULL\n");
            // Wait for a client to disconnect before accepting new connections
            wait(NULL);
            current_clients--;
        }
        size_t readByte =read(fd, &cReq, sizeof(cReq)); 
        // Accept a new client connection
        if (-1 == (int)readByte)
        {
            perror("read");
            exit(1);
        }
        else if (0 == (int)readByte)
            continue;

        printf("gelen pid: %d", (int)cReq.pid);
        if (cReq.type != connect)
        {
            printf("tryConnect/connect hatası\n");
            return -1;
        }
        if (cReq.pid <= 0)
        {
            printf("pid hatası\n");
            return -1;
        }
        printf(">> Client PID %d connected\n", (int)cReq.pid);

        // gelen req'de herhangi bir hata yok devam!
        pid_t pid = fork();
        if (pid == -1)
        {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0)
        { // Child process

            char tempBuff[20];
            sprintf(tempBuff, "%d", (int)getpid());
            clientFifo = malloc(sizeof(tempBuff) + 1);
            sprintf(clientFifo, "cl_%s", tempBuff);
            // hata yok bir client fifo açılabilir
            if (mkfifo(clientFifo, 0666) == -1 && errno != EEXIST)
            {
                // printf("Error while creating the fifo2\n");
                perror("mkfifo");
                exit(6);
            }

            handle_client(current_clients + 1);
            unlink(clientFifo);
        }
        else
        { // Parent process
            current_clients++;
            // Record client activity in log file
            //! LOG gelecek buraya
            // char log_msg[MAX_BUF_SIZE];
            // snprintf(log_msg, sizeof(log_msg), "Client PID %d connected\n", pid);
            // write(log_fd, log_msg, strlen(log_msg));
        }
    }

    close(fd);
    unlink(SERVER_FIFO);
    free(clientFifo);
    return 0;
}

/*  Server pseudo code

    serverfifo aç

    while(childCount<=MAXCHILDCOUNT)
    {
        if(read(serverfifo)) //eğer connection geldiyse
        {
            deserialize coming data

            if(fork())
                childHandler()
            else //in parent process
                log operations
                childCount++
        }



    }


    childHandler():
        string fifoname = "cl_pid"
        mkfifo(fifoname)
        while(comut = read()) //artık burda clientten gelecek komutları dinle
            comutHandler(comut)



    comutHandler(string comut):
        char* [3] cp = comutParser(comut)
        switch(cp[0])
            help_commut()
            list()
            .
            .
            .

*/