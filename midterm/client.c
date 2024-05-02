#include "src/helper.h"

int connected = 0;

// void sigint_handler(int sig)
// {

//     char log_msg[1024];
//     snprintf(log_msg, sizeof(log_msg), "Server shutting down\n");
//     printf("Server shutting down\n");
//     // log_message(log_msg);

//     for (int i = 0; i < (*connectedClientN); i++)
//     {
//         kill(childPids[i], SIGKILL);
//         kill(connectedPids[i], SIGTERM);
//     }

//     // close_server();
//     free(connectedPids);
//     free(childPids);
//     free(connectedClientN);
//     exit(0);
// }

void sigterm_handler(int sig)
{

    printf("Shutting down... (Server shut down)\n");
    exit(0);
}

/// @brief server'a connection requesti gönderir
void makeConnect()
{

    int fd = open(SERVER_FIFO, O_WRONLY);
    printf("0");
    fflush(stdout);
    if (fd < 0)
    {
        perror("open");
        exit(2);
    }

    ConnectionReq cReq;
    cReq.pid = getpid();
    cReq.type = connect;
    // önce connection request atılır:

    if (-1 == write(fd, &cReq, sizeof(cReq)))
    {
        perror("write");
        exit(3);
    }
    close(fd);
    // unlink(SERVER_FIFO);

    // cevap için şimdi clientFifoyu dinle (client fifo açılmış olmalı)
    // eğer cevap gelirse artık bu fifo üzerinden iletişim sağlanacak
    char tempBuff[20];
    sprintf(tempBuff, "%d", (int)getpid());
    clientFifo = malloc(sizeof(tempBuff) + 1);
    sprintf(clientFifo, "cl_%s", tempBuff);

    // hangisi önce açarsa artık
    if (mkfifo(clientFifo, 0666) == -1 && errno != EEXIST)
    {
        // printf("Error while creating the fifo2\n");
        perror("mkfifo");
        exit(6);
    }
    fd = open(clientFifo, O_RDONLY);

    if (fd < 0)
    {
        perror("open");
        exit(2);
    }

    if (-1 == read(fd, &cReq, sizeof(cReq)))
    {
        perror("write");
        exit(3);
    }

    if (cReq.type == established)
    {
        printf("bağlantı sağlandı\n");
        connected = 1;
    }
    else
    {
        printf("bağlantı sağlanaMADI!!\n");
        printf("%d\n", (int)cReq.pid);
    }

    close(fd);
}
// take from console and sends the reuqests after connection initialized
void RequestHandler()
{
    //1- clientFifo'yu yazmak için aç. komut yazılacak
    int clientFifoFd_w = open(clientFifo, O_WRONLY| O_CREAT, 0666);
    if(clientFifoFd_w==-1){
        perror("open clientFifo_w");
        exit(1);
    }
    // Request req = write(clientFifo_w,)

}
int main(int argc, char *argv[])
{
    // if (argc != 3) {
    //     printf("Usage: %s <dirname> <max. #ofClients>\n", argv[0]);
    //     exit(EXIT_FAILURE);
    // }

    // // Create the specified directory if it doesn't exist
    // char *dirname = argv[1];
    // if (mkdir(dirname, 0777) == -1 && errno != EEXIST) {
    //     perror("mkdir");
    //     exit(EXIT_FAILURE);
    // }
    signal(SIGTERM, sigterm_handler);

    printf("pid: %d\n", (int)getpid());

    makeConnect();
    if (connected)
        RequestHandler();

    free(clientFifo);
    return 0;
}