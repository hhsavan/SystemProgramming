#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <semaphore.h>
#include <signal.h>
#include "fifo_seqnum.h"

void handleSignal(int signal)
{
    if (signal == SIGINT)
    {

        printf("Server'dan kill komutu geldi...\n");
        exit(EXIT_SUCCESS); // Terminate the server process
    }
}

//bu quitten gelen için kullanılabilir
void handleSigUSR1(int signal)
{
    if (signal == SIGINT)
    {

        printf("Server'dan kill komutu geldi...\n");
        exit(EXIT_SUCCESS); // Terminate the server process
    }
}
int main(int argc, char const *argv[])
{
    int cmdConnectOption = -1;
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <Connect/tryConnect> <ServerPID>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    if (strcasecmp(argv[1], "tryConnect") == 0)
        cmdConnectOption = TRY_CONNECT;
    else if (strcasecmp(argv[1], "Connect") == 0)
        cmdConnectOption = CONNECT;
    else
    {
        fprintf(stderr, "Invalid option. Must be 'Connect' or 'tryConnect'.\n");
        exit(EXIT_FAILURE);
    }
#pragma region signal
    struct sigaction sa;
    sa.sa_handler = &handleSignal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1)
    {
        fprintf(stderr, "Failed to set signal handler\n");
        exit(EXIT_FAILURE);
    }
#pragma endregion
    int cmdServerPid = atoi(argv[2]);

    /* Create client FIFO  to fetch response from server */
    char clientFifo[CLIENT_FIFO_NAME_LEN];
    snprintf(clientFifo, CLIENT_FIFO_NAME_LEN, CLIENT_FIFO_TEMPLATE, (long)getpid());
    if (mkfifo(clientFifo, S_IRUSR | S_IWUSR) == -1 && errno != EEXIST)
    {
        fprintf(stderr, "Failed to create client FIFO\n");
        exit(EXIT_FAILURE);
    }

    /* Create request client FIFO to send comments to the server */
    char reqClientFifo[CLIENT_FIFO_NAME_LEN];
    snprintf(reqClientFifo, CLIENT_FIFO_NAME_LEN, REQUEST_CLIENT_FIFO_TEMPLATE, (long)getpid());
    if (mkfifo(reqClientFifo, S_IRUSR | S_IWUSR) == -1 && errno != EEXIST)
    {
        fprintf(stderr, "Failed to create request client FIFO\n");
        exit(EXIT_FAILURE);
    }
    struct request req;
    req.pid = getpid();
    req.connectOption = cmdConnectOption;
    strcpy(req.comment, "connectionRequest");
    req.seqLen = 0; // Set to 0 for now, will be updated by the server
    printf("\npid: %d\n", getpid());
    int serverFd = open(SERVER_FIFO, O_WRONLY);
    if (serverFd == -1)
    {
        fprintf(stderr, "Failed to open server FIFO\n");
        exit(EXIT_FAILURE);
    }
    if (write(serverFd, &req, sizeof(struct request)) != sizeof(struct request))
    {
        fprintf(stderr, "Failed to send request to the server\n");
        exit(EXIT_FAILURE);
    }

    /* Open client FIFO for getting response */
    int clientFd = open(clientFifo, O_RDONLY);
    if (clientFd == -1)
    {
        fprintf(stderr, "Failed to open client FIFO\n");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        /* Receive response from the server */
        struct response resp;
        if (read(clientFd, &resp, sizeof(struct response)) != sizeof(struct response))
        {
            fprintf(stderr, "Failed to receive response from the server.\n");
            exit(EXIT_FAILURE);
        }

        printf("server connection: %d", resp.serverConnection);
        printf("server pid: %d", resp.serverPid);
        fflush(stdout);

        if (resp.serverPid != cmdServerPid)
        {
            fprintf(stderr, "Wrong server PID. Failed to connect to server.\n");
            break;
        }

        if (resp.serverConnection == FAILURE && cmdConnectOption == TRY_CONNECT)
        {
            printf("Connection NOT established.\n");
            // clientFifo and reqFIFO remove and unlink yap
            // exit(EXIT_SUCCESS);
        }
        if (resp.serverConnection == SUCCESS)
        {
            while (1)
            {

                char comment[MAX_WORD_SIZE];

                printf("\nEnter command: \n");

                fgets(comment, sizeof(comment), stdin); // read command from stdin

                struct request req;
                req.pid = getpid();
                req.connectOption = cmdConnectOption;
                req.seqLen = 0;
                strcpy(req.comment, comment);
                printf("Request: %s\n", req.comment);

                int reqClientFd = open(reqClientFifo, O_WRONLY);
                if (reqClientFd == -1)
                {
                    fprintf(stderr, "Failed to open request client FIFO\n");
                    exit(EXIT_FAILURE);
                }

                if (write(reqClientFd, &req, sizeof(struct request)) != sizeof(struct request))
                {
                    fprintf(stderr, "Failed to send comment to the request FIFO.\n");
                    exit(EXIT_FAILURE);
                }

                clientFd = open(clientFifo, O_RDONLY);

                /* Receive response from the server */
                struct response resp;
                if (read(clientFd, &resp, sizeof(struct response)) != sizeof(struct response))
                {
                    fprintf(stderr, "Failed to receive response from the server.\n");
                    exit(EXIT_FAILURE);
                }

                if (cmdConnectOption == CONNECT && resp.serverConnection == FAILURE)
                    printf("Waiting in the queue");
                printf("%s\n", resp.commentResult);
            }
        }
    }

    close(clientFd);

    unlink(clientFifo);

    return 0;
}