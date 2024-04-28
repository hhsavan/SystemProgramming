#include "src/helper.h"

int connected = 0;
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
    printf("openı geçti\n");
    if (fd < 0)
    {
        perror("open");
        exit(2);
    }
    printf("waits in read\n");
    if (-1 == read(fd, &cReq, sizeof(cReq)))
    {
        perror("write");
        exit(3);
    }
    printf("readd'ı geçti\n");

    if (cReq.type == established)
    {
        printf("bağlantı sağlandı\n");
        connected = 1;
    }
    else
        printf("bağlantı sağlanaMADI!!\n");

    close(fd);
}
// take from console and sends the reuqests after connection initialized
void sendRequest()
{

    int fd = open(clientFifo, O_WRONLY);
    if (fd < 0)
    {
        perror("open");
        exit(2);
    }

    // şimdi burdan server'a komutlar gönderilecek
    //  if (-1 == read(fd, &cReq, sizeof(cReq)))
    //  {
    //      perror("write");
    //      exit(3);
    //  }
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
    printf("pid: %d\n", (int)getpid());

    makeConnect();
    if (connected)
        sendRequest();

    free(clientFifo);
    return 0;
}