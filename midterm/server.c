#include "src/helper.h"

// Global variables for signal handling
volatile sig_atomic_t received_signal = 0;
pid_t child_pid;

void sigint_handler(int sig)
{

    // char log_msg[1024];
    // snprintf(log_msg, sizeof(log_msg), "Server shutting down\n");
    printf("Server shutting down\n");
    // log_message(log_msg);

    for (int i = 0; i < (*connectedClientN); i++)
    {
        kill(connectedPids[i], SIGTERM);
        kill(childPids[i], SIGKILL);
    }

    // close_server();
    free(connectedPids);
    free(childPids);
    free(connectedClientN);
    exit(0);
}

// Signal handler function
void signal_handler(int signum)
{
    received_signal = signum;
    if (signum == SIGINT)
    {
        printf("Received SIGINT signal. Terminating...\n");
    }
}

/// @brief reads and writes by using clientFifo
void HandleRequest()
{

    Request req;
    int clientFifoFd_r;
    while (1)
    {
        // okuma için aç
        clientFifoFd_r = open(clientFifo, O_RDONLY | O_CREAT, 0666);
        if (clientFifoFd_r == -1)
        {
            perror("open client fifo");
            exit(1);
        }
        // oku
        size_t readByte = read(clientFifoFd_r, &req, sizeof(req));

        if (readByte == -1)
        {
            perror("Read from clientFifoFd_r");
            exit(1);
        }

        if (req.request == KILL)
            printf("kill implement edilecek\n");
        else if (req.request == LIST)
            printf("list implement edilecek\n");
        else if (req.request == READF)
            printf("readf implement edilecek\n");
        else if (req.request == WRITET)
            printf("writet implement edilecek\n");
        else if (req.request == UPLOAD)
            printf("upload implement edilecek\n");
        else if (req.request == DOWNLOAD)
            printf("Download implement edilecek\n");
        else if (req.request == QUIT)
            printf("quit implement edilecek\n");

        if (close(clientFifoFd_r) == -1)
        {
            perror("clientFifoFd_r close");
            exit(1);
        }


    }
}
// Function to handle client connections
int handle_client()
{
    printf("handle client\n");

    if (mkfifo(clientFifo, 0666) == -1 && errno != EEXIST)
    {
        // printf("Error while creating the fifo2\n");
        perror("mkfifo");
        exit(6);
    }
    int fd;
    if ((fd = open(clientFifo, O_WRONLY)) < 0)
    {
        perror("clientFifo open");
        exit(1);
    }

    ConnectionReq cReq;
    cReq.pid = getpid();
    cReq.type = established;
    if (-1 == write(fd, &cReq, sizeof(cReq)))
    {
        perror("write");
        return -1;
    }

    close(fd);

    printf(">> disconnected..\n");
    exit(0); // Exit child process after handling client
}

int main(int argc, char *argv[])
{
    // if (argc != 3)
    // {
    //     printf("Usage: %s <dirname> <max. #ofClients>\n", argv[0]);
    //     exit(EXIT_FAILURE);
    // }

    // Register signal handler for SIGINT (Ctrl-C)
    signal(SIGINT, sigint_handler);

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
    int max_clients = 3; // atoi(argv[2]);
    int current_clients = 0;
    struct ConnectionReq cReq;
    childPids = malloc(MAX_CLIENTS * sizeof(pid_t));
    connectedPids = malloc(MAX_CLIENTS * sizeof(pid_t));
    connectedClientN = malloc(sizeof(pid_t));
    (*connectedClientN) = 0;

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
        if ((*connectedClientN) >= MAX_CLIENTS)
        {
            printf(">> Connection request Que FULL\n");
            // Wait for a client to disconnect before accepting new connections
            // wait(NULL);
            // connectedClientN--;
            //TODO enque: queya ata
        }
        size_t readByte = read(fd, &cReq, sizeof(cReq));
        // Accept a new client connection
        if (-1 == (int)readByte)
        {
            perror("read");
            exit(1);
        }
        else if (0 == (int)readByte)
            continue;

        // printf("gelen pid: %d", (int)cReq.pid);
        if (cReq.type != connect /*|| !=tryconnect*/)
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
            sprintf(tempBuff, "%d", (int)cReq.pid);
            clientFifo = malloc(sizeof(tempBuff) + 1);
            sprintf(clientFifo, "cl_%s", tempBuff);
            // hata yok bir client fifo açılabilir
            // if (mkfifo(clientFifo, 0666) == -1 && errno != EEXIST)
            // {
            //     // printf("Error while creating the fifo2\n");
            //     perror("mkfifo");
            //     exit(6);
            // }
            connectedPids[(*connectedClientN)] = getpid();
            childPids[(*connectedClientN)++] = cReq.pid;
            handle_client();
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