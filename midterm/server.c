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
#include "helper.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include "queue.h"

#define MAX_CLIENTS 10
// pid_t childProcesses[MAX_CLIENTS]; // Array to store child process IDs

// 0: clientpid | 1: childpid | 2: flag (1/0)
int **connectedsArr;
int connectedClients = 0; // Number of connected clients
char logFileName[512];    // Log file name
Queue waitingQueue;       // Queue to store waiting client requests
int g_maxClients;
// sem_t log_mutex;
// sem_t semaphore;

const char *semaphore_name = "/my_named_semaphore";       // Name for the semaphore
const char *serverFifoSemaphore = "/serverFifoSemaphore"; // Name for the semaphore
sem_t *sem;
sem_t *semServer;

// bu parent process tarafından kontrol edilir.
// parent process tüm clientleri kill yapar
void KillServerSignal(int signal)
{
    if (signal == SIGINT)
    {

        // unlink(logFileName);

        for (int i = 0; i < g_maxClients; i++)
        {
            // sadece akriflere signal gönder
            if (connectedsArr[i][2] == 1)
            {
                kill(connectedsArr[i][0], SIGINT); // client process
                kill(connectedsArr[i][1], SIGINT); // child process
            }
        }

        freeQueue(&waitingQueue);
        for (int i = 0; i < g_maxClients; i++)
            free(connectedsArr[i]);
        exit(EXIT_SUCCESS); // Terminate the server process
    }
}

void emptyResp(struct response *resp)
{
    memset(resp->commentResult, 0, 10000);
    resp->commentResult[9999] = '\0';
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

void HandleClientwithAChild(const char *newDirectory, const char *clientFifo, int logFd, int clientPid)
{
    int exitLoop = 0;
    char reqClientFifo[CLIENT_FIFO_NAME_LEN];
    snprintf(reqClientFifo, CLIENT_FIFO_NAME_LEN, REQUEST_CLIENT_FIFO_TEMPLATE, (long)clientPid);
    // printf("BURADA0\n");
    int reqClientFd = open(reqClientFifo, O_RDONLY);
    // printf("BURADA\n");
    if (reqClientFd == -1)
    {
        fprintf(stderr, "Failed to open request client FIFO\n");
        // continue;
    }

    while (1)
    {
        struct request req;

        // printf("BURADA2\n");
        // fflush(stdout);
        // client'a atanmış child process burda yeni komut gelmesini bekler
        int readByt = read(reqClientFd, &req, sizeof(req));
        // printf("BURADq3\n");
        // fflush(stdout);
        if (readByt == -1)
        {
            fprintf(stderr, "Error reading request(Command); discarding\n");
            continue; /* Either partial read or error */
        }
        printf("ComingReq: \n");
        printReq(&req);
        struct response resp;

        char tokens[MAX_TOKENS][MAX_WORD_SIZE];
        int tokenCount = 0;

        tokenCount = parseComment(req.comment, tokens); // parse command
                                                        // printf("tokenCount: %d\n",tokenCount);
                                                        // printf("tokens: %s\n",tokens[0]);
                                                        // printf("tokens: %s\n",tokens[1]);
                                                        // printf("tokens: %s\n",tokens[2]);
                                                        // printf("tokens: %s\n",tokens[3]);
#pragma region parse commands
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

            if (sem_wait(sem) == -1)
            {
                perror("sem_wait");
                exit(EXIT_FAILURE);
            }
            if (dprintf(logFd, "Client%d used help Command (PID: %d)\n", (int)req.pid, (int)getpid()) == -1)
            {
                // basarısız yazma girisimi
                printf("Failure in writing request to server log file\n");
                strcpy(resp.commentResult, "Failure in writing request to server log file\n");
            }
            if (sem_post(sem) == -1)
            {
                perror("sem_post");
                exit(EXIT_FAILURE);
            }
        }
        if (strcmp(tokens[0], "quit") == 0)
        {

            strcpy(resp.commentResult, "Sending write request to server log file\n");

            if (sem_wait(sem) == -1)
            {
                perror("sem_wait");
                exit(EXIT_FAILURE);
            }
            if (dprintf(logFd, "Client%d disconnected (PID: %d)\n", (int)req.pid, (int)getpid()) == -1)
            {
                // basarısız yazma girisimi
                printf("Failure in writing request to server log file\n");
                strcpy(resp.commentResult, "Failure in writing request to server log file\n");
            }
            if (sem_post(sem) == -1)
            {
                perror("sem_post");
                exit(EXIT_FAILURE);
            }

            strcat(resp.commentResult, " waiting for logfile ...");
            strcat(resp.commentResult, " logfile write request granted");
            strcat(resp.commentResult, " bye..");
            struct request disconnectReq;
            disconnectReq.connectOption = DISCONNECT;
            disconnectReq.pid = getppid();

            // dequeue(&waitingQueue);
            sendRequst(SERVER_FIFO, &disconnectReq);
            exitLoop = 1;
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

            if (sem_wait(sem) == -1)
            {
                perror("sem_wait");
                exit(EXIT_FAILURE);
            }
            if (dprintf(logFd, "Client%d used list Command (PID: %d)\n", (int)req.pid, (int)getpid()) == -1)
            {
                // basarısız yazma girisimi
                printf("Failure in writing request to server log file\n");
                strcpy(resp.commentResult, "Failure in writing request to server log file\n");
            }
            if (sem_post(sem) == -1)
            {
                perror("sem_post");
                exit(EXIT_FAILURE);
            }
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

            if (sem_wait(sem) == -1)
            {
                perror("sem_wait");
                exit(EXIT_FAILURE);
            }
            if (dprintf(logFd, "Client%d downloaded this file: %s (PID: %d)\n", (int)req.pid, sourceFileName, (int)getpid()) == -1)
            {
                // basarısız yazma girisimi
                printf("Failure in writing request to server log file\n");
                strcpy(resp.commentResult, "Failure in writing request to server log file\n");
            }
            if (sem_post(sem) == -1)
            {
                perror("sem_post");
                exit(EXIT_FAILURE);
            }
        }
        if (strcmp(tokens[0], "upload") == 0)
        {

            if (sem_wait(sem) == -1)
            {
                perror("sem_wait");
                exit(EXIT_FAILURE);
            }
            if (dprintf(logFd, "Client%d used upload this file: %s (PID: %d)\n", (int)req.pid, "sourceFileName", (int)getpid()) == -1)
            {
                // basarısız yazma girisimi
                printf("Failure in writing request to server log file\n");
                strcpy(resp.commentResult, "Failure in writing request to server log file\n");
            }
            if (sem_post(sem) == -1)
            {
                perror("sem_post");
                exit(EXIT_FAILURE);
            }
        }
        if (strcmp(tokens[0], "killServer") == 0)
        {
            strcpy(resp.commentResult, "\nbye");
            printf("kill signal from client (%ld).. \nterminating..\n", (long)req.pid);

            if (sem_wait(sem) == -1)
            {
                perror("sem_wait");
                exit(EXIT_FAILURE);
            }
            if (dprintf(logFd, "Client%d send killServer (PID: %d)\n", (int)req.pid, (int)getpid()) == -1)
            {
                // basarısız yazma girisimi
                printf("Failure in writing request to server log file\n");
                strcpy(resp.commentResult, "Failure in writing request to server log file\n");
            }
            if (sem_post(sem) == -1)
            {
                perror("sem_post");
                exit(EXIT_FAILURE);
            }
            if (kill(getppid(), SIGINT) == -1)
            {
                perror("Failed to send SIGINT signal to Server");
                return;
            }
            exit(EXIT_SUCCESS);
        }
        else if (strcmp(tokens[0], "readF") == 0)
        {
            // printRes(&req);
            char *fileName = tokens[1];
            int line = atoi(tokens[2]);
            // open client FIFO for writing
            int client_fifo_fd2 = open(clientFifo, O_WRONLY | O_CREAT, 0666);
            if (client_fifo_fd2 == -1)
            {
                perror("open client fifo");
                exit(1);
            }

            char log_msg[1024];
            printf("Client %d requested line %d on %s\n", req.pid, line, fileName);
            // log_message(log_msg);

            char file_path[512];
            strcpy(file_path, newDirectory);
            // strcat(file_path, "/");
            strcat(file_path, fileName);
            char *readed;
            FILE *fp = fopen(file_path, "r");
            printf("285\n");
            readed = get_line(fp, line);
            printf("286\n");
            if (fp == NULL)
            {
                char *error_msg = "Invalid file name!";
                if (write(client_fifo_fd2, error_msg, strlen(error_msg)) == -1)
                {
                    perror("write");
                    close(client_fifo_fd2);
                    exit(1);
                }
            }

            else
            {
                char semaphore_name1[256];
                strcpy(semaphore_name1, "/");
                strcat(semaphore_name1, fileName);
                printf("semaphore_name: %s\n", semaphore_name1);
                fflush(stdout);
                sem_t *semaphore = sem_open(semaphore_name1, O_CREAT, 0644, 1);
                sem_wait(semaphore);
                readed = get_line(fp, line);
                printf("readed sonucu: Allah belanı versin: %s", readed);
                sem_post(semaphore);
                sem_close(semaphore);
            }

            if (readed == NULL)
            {
                char *error_msg = "Invalid line number!";
                if (write(client_fifo_fd2, error_msg, strlen(error_msg)) == -1)
                {
                    perror("write");
                    free(readed);
                    close(client_fifo_fd2);
                    exit(1);
                }
            }

            else
            {
                printf("Read: %s", readed);
                if (write(client_fifo_fd2, readed, strlen(readed)) == -1)
                {
                    perror("write");
                    close(client_fifo_fd2);
                    // (*child_num)--;
                    exit(1);
                }

                free(readed);
            }

            if (close(client_fifo_fd2) == -1)
            {
                perror("close");
                free(readed);
                exit(1);
            }

            if (sem_wait(sem) == -1)
            {
                perror("sem_wait");
                exit(EXIT_FAILURE);
            }
            if (dprintf(logFd, "Client%d read this file: %s (PID: %d)\n", (int)req.pid, fileName, (int)getpid()) == -1)
            {
                // basarısız yazma girisimi
                printf("Failure in writing request to server log file\n");
                strcpy(resp.commentResult, "Failure in writing request to server log file\n");
            }
            if (sem_post(sem) == -1)
            {
                perror("sem_post");
                exit(EXIT_FAILURE);
            }
        }
        else if (strcmp(tokens[0], "archServer") == 0)
        {

            char *tars[] = {"tar", "cvf", tokens[1], ".", NULL};
            pid_t pid = fork();

            if (pid == -1)
            {
                perror("fork");
                exit(EXIT_FAILURE);
            }
            else if (pid == 0)
            {
                execvp("tar", tars);
                perror("execvp");
                exit(EXIT_FAILURE);
            }
            else
            {
                int status;
                waitpid(pid, &status, 0);
                if (WIFEXITED(status))
                    printf("Archive created successfully.\n");
                else
                    printf("Archive creation failed.\n");
            }

            if (sem_wait(sem) == -1)
            {
                perror("sem_wait");
                exit(EXIT_FAILURE);
            }
            if (dprintf(logFd, "Client%d created archfile: %s (PID: %d)\n", (int)req.pid, tokens[1], (int)getpid()) == -1)
            {
                // basarısız yazma girisimi
                printf("Failure in writing request to server log file\n");
                strcpy(resp.commentResult, "Failure in writing request to server log file\n");
            }
            if (sem_post(sem) == -1)
            {
                perror("sem_post");
                exit(EXIT_FAILURE);
            }
        }
        else
            printf("\n");
#pragma endregion

        // commandler handle edildi şimdi responsu gönder
        resp.serverConnection = SUCCESS; // baglandık
        resp.seqNum = connectedClients;
        resp.serverPid = getppid();
        // printf("BURADq4\n");
        // fflush(stdout);

        // int clientFd = open(clientFifo, O_WRONLY);
        // printf("BURADq5\n");
        // if (clientFd == -1)
        // {
        //     fprintf(stderr, "Failed to open request client FIFO\n");
        //     continue;
        // }
        /* Response is sending to the client via clientFIFO */
        int clientFd = open(clientFifo, O_WRONLY);
        // printf("BURADq5\n");
        fflush(stdout);
        if (clientFd == -1)
        {
            fprintf(stderr, "Failed to open request client FIFO\n");
            // continue;
        }
        // printf("BURADq6\n");
        int t = write(clientFd, &resp, sizeof(resp));
        if (t == -1)
        {
            fprintf(stderr, "Error writing to FIFO %s\n", clientFifo);
            if (close(clientFd) == -1)
            {
                fprintf(stderr, "Failed to close client FIFO\n");
                exit(EXIT_FAILURE);
            }
            exit(EXIT_FAILURE);
        }
        // printf("BURADq7\n");
        fflush(stdout);
        if (close(clientFd) == -1)
        {
            fprintf(stderr, "Failed to close client FIFO\n");
            exit(EXIT_FAILURE);
        }
        // printf("BURADq8\n");
        fflush(stdout);
        emptyResp(&resp);

        // quit gelmişse döngüyü kır
        // client'e kill gönder
        if (exitLoop)
        {
            kill(req.pid, SIGINT);
            break;
        }
    }
}

int main(int argc, char const *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <dirname> <max. #ofClients>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    // sem_init(&semaphore, 0, 1); // Initial value is set to 1
    // Create or open the named semaphore
    sem = sem_open(semaphore_name, O_CREAT, 0644, 1);
    if (sem == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    semServer = sem_open(serverFifoSemaphore, O_CREAT, 0644, 1);
    if (semServer == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    const char *dirname = argv[1];
    int maxClients = atoi(argv[2]);
    g_maxClients = maxClients;
    // çalışan clientları tutacağımız 2d array:
    connectedsArr = (int **)malloc(sizeof(int *) * maxClients);
    if (connectedsArr == NULL)
    {
        printf("Memory allocation failed\n");
        return 1;
    }
    for (int i = 0; i < maxClients; i++)
    {
        connectedsArr[i] = (int *)malloc(3 * sizeof(int));
        if (connectedsArr[i] == NULL)
        {
            printf("Memory allocation failed\n");
            return 1;
        }
    }

#pragma region fifoKurulumFalan
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
#pragma endregion

#pragma region signal
    struct sigaction sa;
    sa.sa_handler = &KillServerSignal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1)
    {
        fprintf(stderr, "Failed to set signal handler\n");
        exit(EXIT_FAILURE);
    }
#pragma endregion

    /* Initialize the waiting queue */
    // beekleyyenler buraya atılacak
    initializeQueue(&waitingQueue);

    struct request req;
#pragma region ServerFifoDinlemeKısmı
    int serverFd = open(SERVER_FIFO, O_RDONLY);
    if (serverFd == -1)
    {
        fprintf(stderr, "Failed to open server FIFO\n");
        exit(EXIT_FAILURE);
    }
    // int readByte = read(serverFd, &req, sizeof(struct request));
    // if (readByte == -1)
    // {
    //     fprintf(stderr, "Error reading request; discarding\n");
    //     // continue; /* Either partial read or error */
    // }

#pragma endregion
    while (1)
    {
        // printf("readde bekliyor\n");

        // if (sem_wait(semServer) == -1)
        // {
        //     perror("sem_wait");
        //     exit(EXIT_FAILURE);
        // }
        int readByte = read(serverFd, &req, sizeof(struct request));
        // sem_post(&semaphore);
        // printf("readi geçti\n");
        if (readByte == -1)
        {
            fprintf(stderr, "Error reading request; discarding\n");
            // continue; /* Either partial read or error */
        }
        // if (sem_post(semServer) == -1)
        // {
        //     perror("sem_post");
        //     exit(EXIT_FAILURE);
        // }

        //! GEREK VAR MI
        if (readByte == 0)
            continue;
        char clientFifo[CLIENT_FIFO_NAME_LEN];
        snprintf(clientFifo, CLIENT_FIFO_NAME_LEN, CLIENT_FIFO_TEMPLATE, (long)req.pid);
        // printf("Gelen req: \n");
        // printReq(&req);
        // printf("...\n");
        // clientfifo üzerinden server responsunu cliente verir

        // printf("burda\n");
        // bir client disconnect gönderdi şimdi onu çıkar ve yerine başkasını koy
        if (req.connectOption == DISCONNECT)
        {
            // int i = 0;

            // // 1- connectedArr'den bu clienti çıkar
            // for (; i < g_maxClients; i++)
            // {
            //     // çıkarılacak client bulundu
            //     if (connectedsArr[i][0] == req.pid && connectedsArr[i][2] == 1)
            //     {
            //         connectedsArr[i][0] = -1;
            //         // bunun aşağıda yapılması lazım. çünkü henüz child process oluşturlmadı
            //         connectedsArr[i][0] = -1;
            //         connectedsArr[i][2] = 0; // deaktif yap

            //         break;
            //     }
            // }
            // // 2- waitingQueuden deque yap ve geleni çıkan yere yerleştir
            // pid_t aWaitingClient = dequeue(&waitingQueue);
            // if (aWaitingClient != -1)
            // {
            //     connectedsArr[i][0] = aWaitingClient;
            //     connectedsArr[i][2] = 0; // aktif yap
            // }

            // // 3- connectedArr'a gelen bu clienti fork ile çalıştır
            // int tpid = fork();
            // // child process içinde
            // if (tpid == 0)
            // {
            //     connectedsArr[i][1] = getpid(); // client'a hizmet eden client'in pid'si
            //     HandleClientwithAChild(newDirectory, clientFifo, logFd, aWaitingClient);
            // }
            // else
            // {
            //     // server'a bağlantı hakkında client'e bilgi verir
            //     sendThatConnectionEstablishedtoClient(clientFifo);
            //     //? SEMAPHORE LAZIM
            //     // connectedsArr[connectedClients][1] = getpid(); //clienti de ekle
            //     connectedClients++;
            //     // TODO SEMAPHORE END

            //     printf("Client PID %ld connected as "
            //            "client%d"
            //            "\n",
            //            (long)req.pid, connectedClients);
            //     fflush(stdout);
            //     /* Log the client connection */
            //     dprintf(logFd, "Client connected (PID: %d)\n", req.pid);
            // }
        }
        // try connect ama server ful. bunu alan client çıkış yapacak
        else if (req.connectOption == TRY_CONNECT && connectedClients >= maxClients)
        {

            //! Doluysa clientHandler fonksiyonu biir parametre ile çağırılacak o da cevap verecek
            // struct response resp;
            // resp.serverConnection = FAILURE; /* Indicate server is full */

            // if (write(clientFd, &resp, sizeof(struct response)) != sizeof(struct response))
            // {
            //     fprintf(stderr, "Error writing to FIFO %s\n", clientFifo);
            //     // if (close(clientFd) == -1)
            //     // {
            //     //     fprintf(stderr, "Failed to close client FIFO\n");
            //     //     exit(EXIT_FAILURE);
            //     // }
            //     //! burda yazma olmadıysa ne yapmalı?
            //     continue;
            // }

            // if (close(clientFd) == -1)
            // {
            //     fprintf(stderr, "Failed to close client FIFO\n");
            //     continue;
            // }
        }

        else if (req.connectOption == CONNECT)
        {

            // server dolu. client'w söylle beklesin
            //  gelen client bilgilerini queue ye ekle
            if (connectedClients >= maxClients) // Server'da yer yok
            {

                enqueue(&waitingQueue, req.pid);
                struct response resp;
                resp.serverConnection = WAIT; /* Indicate server is full */
                resp.serverPid = getpid();
                resp.seqNum = connectedClients;
                strcpy(resp.commentResult, "bekle güzel kardeşim");
                // printf("bekle güzel kardeşim\n");

                // if (write(clientFd, &resp, sizeof(resp)) == -1)
                // {
                //     fprintf(stderr, "Error writing to FIFO %s\n", clientFifo);
                //     // if (close(clientFd) == -1)
                //     // {
                //     //     fprintf(stderr, "2Failed to close client FIFO\n");
                //     //     exit(EXIT_FAILURE);
                //     // }

                //     continue;
                // }

                // if (close(clientFd) == -1)
                // {
                //     fprintf(stderr, "Failed to close client FIFO\n");
                //     exit(EXIT_FAILURE);
                // }
                printf("Client connection attempt (PID: %d, server full)\n", req.pid);

                if (sem_wait(sem) == -1)
                {
                    perror("sem_wait");
                    exit(EXIT_FAILURE);
                }
                if (dprintf(logFd, "Client connection attempt (PID: %d, server full)\n", req.pid) == -1)
                {
                    // basarısız yazma girisimi
                    printf("Failure in writing request to server log file\n");
                    strcpy(resp.commentResult, "Failure in writing request to server log file\n");
                }
                if (sem_post(sem) == -1)
                {
                    perror("sem_post");
                    exit(EXIT_FAILURE);
                }
            }
            else //! serverda yer var. bağlantı sağlandı. connectedArra ekle
            {
                connectedsArr[connectedClients][0] = req.pid;
                connectedsArr[connectedClients][2] = 1; // 1 olması sistemde çalıştığını gösterir
                /* Fork a child process */
                pid_t childPid = fork();

                if (childPid == -1)
                {
                    fprintf(stderr, "Failed to fork child process. Try again\n");
                    exit(EXIT_FAILURE);
                    // continue;
                }
                else if (childPid == 0) // child process
                {

                    // TODO SEMAPHORE LAZIM

                    connectedsArr[connectedClients][1] = getpid(); // clienti de ekle
                                                                   // connectedClients++;
                                                                   // TODO SEMAPHORE END
                    sendThatConnectionEstablishedtoClient(clientFifo);
                    HandleClientwithAChild(newDirectory, clientFifo, logFd, req.pid);
                    //! LOGFd close edilmeli heryerde
                    // TODO
                    freeQueue(&waitingQueue);
                    for (int i = 0; i < maxClients; i++)
                        free(connectedsArr[i]);

                    //!
                    // raise(SIGINT);
                    return 0;
                }
                else // parent process
                {
                    // server'a bağlantı hakkında client'e bilgi verir
                    // sendThatConnectionEstablishedtoClient(clientFifo);
                    //? SEMAPHORE LAZIM
                    // connectedsArr[connectedClients][1] = getpid(); //clienti de ekle
                    connectedClients++;
                    // TODO SEMAPHORE END
                    // closew(clientFd);
                    printf("Client PID %ld connected as "
                           "client%d"
                           "\n",
                           (long)req.pid, connectedClients);
                    fflush(stdout);
                    /* Log the client connection */

                    if (sem_wait(sem) == -1)
                    {
                        perror("sem_wait");
                        exit(EXIT_FAILURE);
                    }
                    if (dprintf(logFd, "Client connected (PID: %d)\n", req.pid) == -1)
                    {
                        // basarısız yazma girisimi
                        printf("Failure in writing request to server log file\n");
                        // strcpy(resp.commentResult, "Failure in writing request to server log file\n");
                    }
                    if (sem_post(sem) == -1)
                    {
                        perror("sem_post");
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }

        else
        {
            fprintf(stderr, "Invalid request received\n");
        }
    }
    //! LOGFd close edilmeli heryerde
    freeQueue(&waitingQueue);
    for (int i = 0; i < maxClients; i++)
        free(connectedsArr[i]);
    // sem_destroy(&semaphore);
    // Close and unlink the semaphore
    if (sem_close(sem) == -1)
    {
        perror("sem_close");
        exit(EXIT_FAILURE);
    }

    if (sem_unlink(semaphore_name) == -1)
    {
        perror("sem_unlink");
        exit(EXIT_FAILURE);
    }

    if (sem_close(semServer) == -1)
    {
        perror("sem_close");
        exit(EXIT_FAILURE);
    }

    if (sem_unlink(serverFifoSemaphore) == -1)
    {
        perror("sem_unlink");
        exit(EXIT_FAILURE);
    }
    return 0;
}

void sendThatConnectionEstablishedtoClient(const char *clientFifo)
{

#pragma region clientaBilgi
    /* Parent process */
    struct response resp;
    resp.serverConnection = SUCCESS; // baglandık
    resp.seqNum = connectedClients;
    resp.seqNum = 0;
    strcpy(resp.commentResult, "Connected to the server.\n");
    // resp.commentResult = "cmcm";
    resp.serverPid = getppid();
    // printf("main processte: \n");
    // printReq(&resp);
    int clientFd = open(clientFifo, O_WRONLY);
    if (clientFd == -1)
    {
        fprintf(stderr, "Failed to open request client FIFO\n");
        // continue;
        return; //?
    }
    // printf("\n\nsendThatConnectionEstablishedtoClient'dan gönderilen\n");
    // printRes(&resp);
    // printf("\n\n");
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

// fifonun adını alır. fifoyu açar. requesti oraya yazar fifoyu kapatır ve döner
void sendRequst(const char *fifo, const struct request *req)
{
    // printf("client'a Gönderilecek exit req:\n");
    // printReq(req);
    int fd = open(fifo, O_WRONLY);
    if (fd == -1)
    {
        fprintf(stderr, "2-Failed to open request client FIFO\n");
        closew(fd);
        return;
    }
    /* bağlandık mı is sending to the client via clientFIFO */
    if (write(fd, &req, sizeof(req)) != sizeof(req))
    {
        fprintf(stderr, "1-Failed to open request client FIFO\n");
        closew(fd);
        return;
    }
    // printf("Gönderildi aldın mı\n");

    closew(fd);
}
// close wrapper function
void closew(int fd)
{
    if (close(fd) == -1)
    {
        fprintf(stderr, "Failed to close client FIFO...\n");
        exit(EXIT_FAILURE);
    }
}

char *get_line(FILE *fp, int line_num)
{
    // printf("get_line içinde line_num: %d\n", line_num);
    if (fp == NULL || line_num <= 0)
    {
        return NULL; // Invalid input parameters
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int current_line_num = 0;

    // Reset file pointer to the beginning of the file
    rewind(fp);

    while ((read = getline(&line, &len, fp)) != -1)
    {
        current_line_num++;

        if (current_line_num == line_num)
        {
            // found the requested line
            printf("line: %s", line);
            fclose(fp);
            free(line); // Free memory allocated by getline
            fclose(fp);
            return line;
        }
    }

    fclose(fp);
    free(line);  // Free memory allocated by getline
    return NULL; // Line number out of range
}
