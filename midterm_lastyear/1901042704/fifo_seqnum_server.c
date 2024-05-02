#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <semaphore.h>
#include <dirent.h>
#include <signal.h>
#include "fifo_seqnum.h"
#include "queue.h"

#define MAX_CLIENTS 10
pid_t childProcesses[MAX_CLIENTS]; // Array to store child process IDs
int **connectedClientsArr;
int connectedClients = 0; // Number of connected clients
char logFileName[256];    // Log file name
Queue waitingQueue;       // Queue to store waiting client requests

void handleSignal(int signal)
{
    if (signal == SIGINT)
    {

        unlink(logFileName);

        for (int i = 0; i < connectedClients; i++)
        {
            kill(childProcesses[i], SIGINT);
        }

        freeQueue(&waitingQueue);

        exit(EXIT_SUCCESS); // Terminate the server process
    }
}

int parseComment(const char *input, char tokens[][MAX_WORD_SIZE])
{

    int tokenCount = 0;
    char tempInput[MAX_WORD_SIZE];
    strncpy(tempInput, input, sizeof(tempInput));

    // Remove the trailing newline character, if present
    tempInput[strcspn(tempInput, "\n")] = '\0';

    // Tokenize the string based on spaces
    char *token = strtok(tempInput, " ");
    while (token != NULL && tokenCount < MAX_TOKENS)
    {
        strcpy(tokens[tokenCount], token);
        tokenCount++;
        token = strtok(NULL, " ");
    }

    return tokenCount;
}

int main(int argc, char const *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <dirname> <max. #ofClients>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *dirname = argv[1];
    int maxClients = atoi(argv[2]);

    //çalışan clientları tutacağımız 2d array:
    connectedClientsArr = (int **)malloc(sizeof(int *) * maxClients);
    if (connectedClientsArr == NULL)
    {
        printf("Memory allocation failed\n");
        return 1;
    }
    for (int i = 0; i < maxClients; i++)
    {
        connectedClientsArr[i] = (int *)malloc(2 * sizeof(int));
        if (connectedClientsArr[i] == NULL)
        {
            printf("Memory allocation failed\n");
            return 1;
        }
    }
    
    
    /* Create server FIFO */
    umask(0);
    if (mkfifo(SERVER_FIFO, S_IRUSR | S_IWUSR | S_IWGRP) == -1 && errno != EEXIST)
    {
        fprintf(stderr, "Failed to create server FIFO\n");
        exit(EXIT_FAILURE);
    }

    printf("Server Started PID %ld...\n", (long)getpid());
    printf("Waiting for clients...\n");

    if (mkdir(dirname, S_IRWXU | S_IRWXG | S_IRWXO) == -1 && errno != EEXIST)
    {
        perror("mkdir");
        exit(EXIT_FAILURE);
    }

    // char* path = "./Here";
    char path[50];
    snprintf(path, sizeof(path), "./%s", dirname);
    char currentDir[250];
    if (getcwd(currentDir, sizeof(currentDir)) != NULL)
        printf("\n");
    else
    {
        perror("Failed to get current directory");
        exit(EXIT_FAILURE);
    }

    /* Change directory to the specified directory */
    if (chdir(path) == -1)
    {
        perror("chdir");
        exit(EXIT_FAILURE);
    }

    char newDirectory[250];
    if (getcwd(newDirectory, sizeof(newDirectory)) != NULL)
        printf("\n");
    else
    {
        perror("Failed to get current directory");
        exit(EXIT_FAILURE);
    }

    snprintf(logFileName, sizeof(logFileName), "%s/server_log.txt", newDirectory);
    int logFd = open(logFileName, O_WRONLY | O_CREAT | O_APPEND, S_IRWXU);
    if (logFd == -1)
    {
        fprintf(stderr, "Failed to open log file\n");
        exit(EXIT_FAILURE);
    }
    // printf("log file oluşturuldu: %s\n", logFileName);

    struct sigaction sa;
    sa.sa_handler = &handleSignal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1)
    {
        fprintf(stderr, "Failed to set signal handler\n");
        exit(EXIT_FAILURE);
    }

    /* Initialize the waiting queue */
    // beekleyyenler buraya atılacak
    initializeQueue(&waitingQueue);

    while (1)
    {

        struct request req;

        int serverFd = open(SERVER_FIFO, O_RDONLY);
        if (serverFd == -1)
        {
            fprintf(stderr, "Failed to open server FIFO\n");
            exit(EXIT_FAILURE);
        }

        if (read(serverFd, &req, sizeof(struct request)) != sizeof(struct request))
        {
            fprintf(stderr, "Error reading request; discarding\n");
            // continue; /* Either partial read or error */
        }
        printf("Gelen req: \n");
        printReq(&req);
        printf("...\n");

        // clientfifo üzerinden server responsunu cliente verir
        char clientFifo[CLIENT_FIFO_NAME_LEN];
        snprintf(clientFifo, CLIENT_FIFO_NAME_LEN, CLIENT_FIFO_TEMPLATE, (long)req.pid);
        int clientFd = open(clientFifo, O_WRONLY);
        if (clientFd == -1)
            fprintf(stderr, "Failed to open client FIFO\n");

        // try connect ama server ful. bunu alan client çıkış yapacak
        if (req.connectOption == TRY_CONNECT && connectedClients >= maxClients)
        {

            struct response resp;
            resp.serverConnection = FAILURE; /* Indicate server is full */

            if (write(clientFd, &resp, sizeof(struct response)) != sizeof(struct response))
            {
                fprintf(stderr, "Error writing to FIFO %s\n", clientFifo);
                // if (close(clientFd) == -1)
                // {
                //     fprintf(stderr, "Failed to close client FIFO\n");
                //     exit(EXIT_FAILURE);
                // }
                //! burda yazma olmadıysa ne yapmalı?
                continue;
            }

            if (close(clientFd) == -1)
            {
                fprintf(stderr, "Failed to close client FIFO\n");
                continue;
            }
        }

        else if (req.connectOption == CONNECT)
        {
            // server dolu. client'w söylle beklesin
            //  gelen client bilgilerini queue ye ekle
            if (connectedClients >= maxClients) // Server'da yer yok
            {

                enqueue(&waitingQueue, req.pid);
                struct response resp;
                resp.serverConnection = SUCCESS; /* Indicate server is full */

                if (write(clientFd, &resp, sizeof(struct response)) != sizeof(struct response))
                {
                    fprintf(stderr, "Error writing to FIFO %s\n", clientFifo);

                    // if (close(clientFd) == -1)
                    // {
                    //     fprintf(stderr, "Failed to close client FIFO\n");
                    //     exit(EXIT_FAILURE);
                    // }
                    continue;
                }

                if (close(clientFd) == -1)
                {
                    fprintf(stderr, "Failed to close client FIFO\n");
                    exit(EXIT_FAILURE);
                }

                dprintf(logFd, "Client connection attempt (PID: %d, server full)\n", req.pid);
            }
            else // serverda yer var.
            {

                /* Fork a child process */
                pid_t childPid = fork();

                if (childPid == -1)
                {
                    fprintf(stderr, "Failed to fork child process. Try again\n");
                    continue;
                }
                else if (childPid == 0) // child process
                {

                    while (1)
                    {

                        char reqClientFifo[CLIENT_FIFO_NAME_LEN];
                        snprintf(reqClientFifo, CLIENT_FIFO_NAME_LEN, REQUEST_CLIENT_FIFO_TEMPLATE, (long)req.pid);
                        int reqClientFd = open(reqClientFifo, O_RDONLY);
                        if (reqClientFd == -1)
                        {
                            fprintf(stderr, "Failed to open request client FIFO\n");
                            continue;
                        }

                        if (read(reqClientFd, &req, sizeof(struct request)) != sizeof(struct request))
                        {
                            fprintf(stderr, "Error reading request(comment); discarding\n");
                            continue; /* Either partial read or error */
                        }
                        printf("Req:\n");
                        printReq(&req);
                        struct response resp;

                        char tokens[MAX_TOKENS][MAX_WORD_SIZE];
                        int tokenCount = 0;

                        tokenCount = parseComment(req.comment, tokens); // parse command

#pragma region parse comments
                        if (strcmp(tokens[0], "help") == 0)
                        {
                            if (strcmp(tokens[1], "readF") == 0)
                                strcpy(resp.commentResult, "\n\nreadF <file> <line #>\ndisplay the #th line of the <file>, returns with an\nerror if <file> does not exists\n");
                            else if (strcmp(tokens[1], "writeT") == 0)
                                strcpy(resp.commentResult, "\n\n<file> <line #> <string> :\nrequest to write the content of “string” to the #th line the <file>, if the line # is not given\nwrites to the end of file\n");
                            else if (strcmp(tokens[1], "upload") == 0)
                                strcpy(resp.commentResult, "\n\nupload <file>\nuploads the file from the current working directory of client to the Servers directory\n");
                            else if (strcmp(tokens[1], "download") == 0)
                                strcpy(resp.commentResult, "\n\ndownload <file>\nrequest to receive <file> from Servers directory to client sid\n");
                            else if (strcmp(tokens[1], "quit") == 0)
                                strcpy(resp.commentResult, "\n\nquit\nSend write request to Server side log file and quits\n");
                            else if (strcmp(tokens[1], "killServer") == 0)
                                strcpy(resp.commentResult, "\n\nkillServer\nSends a kill request to the Server\n");
                            else if (strcmp(tokens[1], "list") == 0)
                                strcpy(resp.commentResult, "\n\nlist\nsends a request to display the list of files in Servers director\n");
                            else
                                strcpy(resp.commentResult, "\n\nAvailable comments are: \n help, list, readF, writeT, upload, download, quit, killServer\n");
                        }
                        if (strcmp(tokens[0], "quit") == 0)
                        {

                            dequeue(&waitingQueue);
                            strcpy(resp.commentResult, "Sending write request to server log file\n");
                            if (dprintf(logFd, "Client%d disconnected (PID: %d)\n", (int)req.pid) == -1)
                            {
                                // basarısız yazma girisimi
                                printf("Failure in writing request to server log file\n");
                                strcpy(resp.commentResult, "Failure in writing request to server log file\n");
                            }
                            strcat(resp.commentResult, " waiting for logfile ...");
                            strcat(resp.commentResult, " logfile write request granted");
                            strcat(resp.commentResult, " bye..");
                        }

                        if (strcmp(tokens[0], "list") == 0)
                        {
                            DIR *dir;
                            struct dirent *entry;
                            char *cp;
                            dir = opendir(newDirectory);
                            if (dir == NULL)
                            {
                                perror("opendir");
                                // return 1;
                            }
                            else
                            {
                                strcat(resp.commentResult, "\n");
                                while ((entry = readdir(dir)) != NULL)
                                {
                                    if (strcmp(".", entry->d_name) == 0)
                                        continue;
                                    else if (strcmp("..", entry->d_name) == 0)
                                        continue;
                                    strcat(resp.commentResult, entry->d_name);
                                    strcat(resp.commentResult, "\n");
                                }
                            }
                            closedir(dir);
                        }
                        if (strcmp(tokens[0], "download") == 0)
                        {
                            const char *sourceFileName = tokens[1];          // Name of the file to copy
                            const char *destinationDirectory = newDirectory; // Destination directory path
                            // printf("klkkkkkkkkkkkkkkkkkkkkkkkk %s",newDirectory);
                            const char *destinationFileName = strcat(tokens[1], "(Copy)");
                            // printf("sourceFileName %s",sourceFileName);
                            FILE *sourceFile = fopen(sourceFileName, "rb");
                            if (sourceFile == NULL)
                            {
                                perror("Failed to open source file");
                            }
                            else
                            {
                                // Create the destination file path
                                char *destinationFilePath = (char *)malloc(strlen(destinationDirectory) + strlen(destinationFileName) + 1);
                                strcpy(destinationFilePath, destinationDirectory);
                                strcat(destinationFilePath, destinationFileName);
                                printf("dest %s", destinationFilePath);

                                // Open the destination file in binary mode for writing
                                FILE *destinationFile = fopen(destinationFilePath, "wb");
                                if (destinationFile == NULL)
                                {
                                    perror("Failed to create destination file");
                                    fclose(sourceFile);
                                    free(destinationFilePath);
                                    // return 1;
                                }
                                else
                                {
                                    // Read the contents of the source file and write them to the destination file
                                    char buffer[1024];
                                    size_t bytesRead;
                                    long fileSize = 0;
                                    while ((bytesRead = fread(buffer, 1, sizeof(buffer), sourceFile)) > 0)
                                    {
                                        fwrite(buffer, 1, bytesRead, destinationFile);
                                        fileSize += bytesRead;
                                    }

                                    // Close the files
                                    fclose(sourceFile);
                                    fclose(destinationFile);

                                    free(destinationFilePath);
                                    strcat(resp.commentResult, "file transfer request received. Beginning file transfer:\n");
                                    strcat(resp.commentResult, (char *)fileSize);
                                    strcat(resp.commentResult, " bytes transferred\n");
                                    // printf("\nasdasdasd: %s\n",resp.commentResult);
                                }
                            }
                        }
                        if (strcmp(tokens[0], "upload") == 0)
                        {
                        }
                        if (strcmp(tokens[0], "killServer") == 0)
                        {
                            strcpy(resp.commentResult, "\nbye");
                            printf("kill signal from client (%ld).. terminating..\n", (long)getpid());
                            printf("bye");
                            if (kill(childPid, SIGINT) == -1)
                            {
                                perror("Failed to send SIGINT signal");
                                return 1;
                            }
                            if (kill(getppid(), SIGINT) == -1)
                            {
                                perror("Failed to send SIGINT signal to Server");
                                return 1;
                            }
                            exit(EXIT_SUCCESS);
                        }
                        else
                            printf("\n");
#pragma endregion

                        // commandler handle edildi şimdi responsu gönder
                        resp.serverConnection = SUCCESS; // baglandık
                        resp.seqNum = connectedClients;
                        resp.serverPid = getppid();

                        int clientFd = open(clientFifo, O_WRONLY);
                        if (clientFd == -1)
                        {
                            fprintf(stderr, "Failed to open request client FIFO\n");
                            continue;
                        }
                        /* Response is sending to the client via clientFIFO */
                        if (write(clientFd, &resp, sizeof(struct response)) != sizeof(struct response))
                        {
                            fprintf(stderr, "Error writing to FIFO %s\n", clientFifo);
                            if (close(clientFd) == -1)
                            {
                                fprintf(stderr, "Failed to close client FIFO\n");
                                exit(EXIT_FAILURE);
                            }
                            exit(EXIT_FAILURE);
                        }
                        if (close(clientFd) == -1)
                        {
                            fprintf(stderr, "Failed to close client FIFO\n");
                            exit(EXIT_FAILURE);
                        }
                    }
                }
                else // parent process
                {
#pragma region clientaBilgi
                    /* Parent process */
                    struct response resp;
                    resp.serverConnection = SUCCESS; // baglandık
                    resp.seqNum = connectedClients;
                    resp.seqNum = 0;
                    strcpy(resp.commentResult, "Server'a bağlantı sağlandı.\n");
                    // resp.commentResult = "cmcm";
                    resp.serverPid = getpid();
                    // printf("main processte: \n");
                    // printReq(&resp);
                    int clientFd = open(clientFifo, O_WRONLY);
                    if (clientFd == -1)
                    {
                        fprintf(stderr, "Failed to open request client FIFO\n");
                        continue;
                    }
                    /* bağlandık mı is sending to the client via clientFIFO */
                    if (write(clientFd, &resp, sizeof(struct response)) != sizeof(struct response))
                    {
                        fprintf(stderr, "Error writing to FIFO %s\n", clientFifo);
                        if (close(clientFd) == -1)
                        {
                            fprintf(stderr, "Failed to close client FIFO\n");
                            exit(EXIT_FAILURE);
                        }
                        exit(EXIT_FAILURE);
                    }

                    if (close(clientFd) == -1)
                    {
                        fprintf(stderr, "Failed to close client FIFO\n");
                        exit(EXIT_FAILURE);
                    }
#pragma endregion

                    //! HATA enque yaparkan
                    enqueue(&childProcesses, childPid);
                    connectedClients++;

                    printf("Client PID %ld connected as "
                           "client%d"
                           "\n",
                           (long)childPid, connectedClients);
                    fflush(stdout);
                    /* Log the client connection */
                    dprintf(logFd, "Client connected (PID: %d)\n", req.pid);
                }
            }
        }
        else
        {
            fprintf(stderr, "Invalid request received\n");
        }
    }

    freeQueue(&waitingQueue);
    for(int i=0; i<maxClients; i++)
        free(connectedClientsArr[i]);
    return 0;
}
void printReq(const struct request *str)
{
    printf("pid: %d\n", str->pid);
    printf("seqLen: %d\n", str->seqLen);
    printf("comment: %s\n", str->comment);
    printf("connectOption: %d\n", str->connectOption);
}
void printRes(const struct response *str)
{
    printf("serverPid: %d\n", str->serverPid);
    printf("seqNum: %d\n", str->seqNum);
    printf("commentResult: %s\n", str->commentResult);
    printf("serverConnection: %d\n", str->serverConnection);
}