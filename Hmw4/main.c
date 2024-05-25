#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>
#include <signal.h>

pthread_mutex_t DONEMUT = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t OUTMUT = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t ISEMPTY = PTHREAD_COND_INITIALIZER;
pthread_cond_t ISFULL = PTHREAD_COND_INITIALIZER;
pthread_mutex_t M = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t MAXFDNUM = PTHREAD_COND_INITIALIZER;
pthread_mutex_t FDMUT = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t TMUT = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t REGMUT = PTHREAD_MUTEX_INITIALIZER;

volatile sig_atomic_t SIGINTFLAG = 0;
int COUNT = 0;
long FIFONUM = 0;
long regularFileNum = 0;
long directoryNum = 0;
long long totalByte = 0;
int done = 0;

struct Element {
    char fileName[500];
    int readFd;
    int writeFd;
};

struct Element *buffer;

void sigintHandler(int signum) {
    SIGINTFLAG = 1;
}
void CreateSameFilesInDest(int bufferSize, const char *srcDirName, const char *destDirName);
void cleanup() {
    free(buffer);
    pthread_mutex_destroy(&DONEMUT);
    pthread_mutex_destroy(&OUTMUT);
    pthread_mutex_destroy(&M);
    pthread_mutex_destroy(&FDMUT);
    pthread_mutex_destroy(&TMUT);
    pthread_mutex_destroy(&REGMUT);
    pthread_cond_destroy(&ISEMPTY);
    pthread_cond_destroy(&ISFULL);
    pthread_cond_destroy(&MAXFDNUM);
}

void *manager(void *args) {
    char **arguments = (char **)args;
    int bufferSize = atoi(arguments[0]);
    char *srcPath = arguments[1];
    char *destPath = arguments[2];

    /* Create destination directory if it does not exist */
    struct stat st;
    if (stat(destPath, &st) == -1) {
        if (mkdir(destPath, 0777) == -1) {
            pthread_mutex_lock(&OUTMUT);
            printf("Failed to create %s destination directory\n", destPath);
            pthread_mutex_unlock(&OUTMUT);
        }
    }

    CreateSameFilesInDest(bufferSize, srcPath, destPath);

    pthread_mutex_lock(&M);
    done = 1;
    pthread_cond_broadcast(&ISFULL);
    pthread_mutex_unlock(&M);

    pthread_exit(NULL);
}

void CreateSameFilesInDest(int bufferSize, const char *srcDirName, const char *destDirName) {
    DIR *dir = opendir(srcDirName);
    if (dir == NULL) return;

    struct dirent *dp;
    while ((dp = readdir(dir)) != NULL) {
        if (SIGINTFLAG == 1) {
            closedir(dir);
            return;
        }
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
            continue;

        char path[500] = {0};
        char destPath[500] = {0};
        strcat(path, srcDirName);
        strcat(path, "/");
        strcat(path, dp->d_name);
        strcat(destPath, destDirName);
        strcat(destPath, "/");
        strcat(destPath, dp->d_name);

        if (dp->d_type == 4) {
            struct stat st;
            if (stat(destPath, &st) == -1) {
                if (mkdir(destPath, 0777) == -1) {
                    pthread_mutex_lock(&OUTMUT);
                    printf("Failed to create %s directory!\n", destPath);
                    pthread_mutex_unlock(&OUTMUT);
                }
            }
            directoryNum++;
            CreateSameFilesInDest(bufferSize, path, destPath);
        } else {
            int srcFd = open(path, O_RDONLY);
            if (srcFd == -1) {
                pthread_mutex_lock(&FDMUT);
                if (errno == EMFILE) {
                    pthread_cond_wait(&MAXFDNUM, &FDMUT);
                }
                pthread_mutex_lock(&OUTMUT);
                printf("Failed to open %s source file!\n", path);
                pthread_mutex_unlock(&OUTMUT);
                pthread_mutex_unlock(&FDMUT);
                continue;
            }

            int destFd = open(destPath, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU);
            if (destFd == -1) {
                pthread_mutex_lock(&FDMUT);
                if (errno == EMFILE) {
                    pthread_cond_wait(&MAXFDNUM, &FDMUT);
                }
                pthread_mutex_lock(&OUTMUT);
                printf("Failed to open %s destination file!\n", destPath);
                pthread_mutex_unlock(&OUTMUT);
                pthread_mutex_unlock(&FDMUT);
                close(srcFd);
                continue;
            }

            pthread_mutex_lock(&M);
            while (COUNT == bufferSize) {
                pthread_cond_wait(&ISEMPTY, &M);
            }
            buffer[COUNT].readFd = srcFd;
            buffer[COUNT].writeFd = destFd;
            strcpy(buffer[COUNT].fileName, destPath);
            COUNT++;
            pthread_cond_broadcast(&ISFULL);
            pthread_mutex_unlock(&M);
        }
    }
    closedir(dir);
}

void *worker(void *arg) {
    while (1) {
        if (SIGINTFLAG == 1) {
            pthread_exit(NULL);
        }
        pthread_mutex_lock(&M);
        while (COUNT == 0 && !done) {
            pthread_cond_wait(&ISFULL, &M);
        }

        if (done && COUNT == 0) {
            pthread_mutex_unlock(&M);
            break;
        }

        struct Element element;
        element.readFd = buffer[COUNT - 1].readFd;
        element.writeFd = buffer[COUNT - 1].writeFd;
        strcpy(element.fileName, buffer[COUNT - 1].fileName);

        buffer[COUNT - 1].readFd = -1;
        buffer[COUNT - 1].writeFd = -1;
        buffer[COUNT - 1].fileName[0] = '\0';
        COUNT--;
        pthread_mutex_unlock(&M);

        ssize_t bytesRead;
        int errorFlag = 0;
        do {
            char data[4096];
            memset(data, 0, sizeof(data));
            bytesRead = read(element.readFd, data, 4096);
            if (bytesRead == -1) {
                pthread_mutex_lock(&OUTMUT);
                printf("Error reading %s source file!\n", element.fileName);
                errorFlag = 1;
                pthread_mutex_unlock(&OUTMUT);
                break;
            }

            if (bytesRead > 0) {
                ssize_t bytesWritten = write(element.writeFd, data, bytesRead);
                if (bytesWritten == -1) {
                    pthread_mutex_lock(&OUTMUT);
                    printf("Error writing to %s destination file!\n", element.fileName);
                    errorFlag = 1;
                    pthread_mutex_unlock(&OUTMUT);
                    break;
                }

                pthread_mutex_lock(&TMUT);
                totalByte += bytesWritten;
                pthread_mutex_unlock(&TMUT);
            }
        } while (bytesRead > 0);

        pthread_mutex_lock(&REGMUT);
        regularFileNum++;
        pthread_mutex_unlock(&REGMUT);

        close(element.readFd);
        close(element.writeFd);
        pthread_cond_broadcast(&MAXFDNUM);

        if (!errorFlag) {
            pthread_mutex_lock(&OUTMUT);
            printf("%s has been successfully copied.\n", element.fileName);
            pthread_mutex_unlock(&OUTMUT);
        }

        pthread_cond_broadcast(&ISEMPTY);
    }
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        printf("Usage: ./programName <bufferSize> <consumerNum> <sourcePath> <destinationPath>\n");
        exit(1);
    }

    struct sigaction sa;
    sa.sa_handler = sigintHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    int consumerNum = atoi(argv[2]);
    int bufferSize = atoi(argv[1]);
    char *args[3] = {argv[1], argv[3], argv[4]};

    buffer = (struct Element *)calloc(bufferSize, sizeof(struct Element));
    pthread_t prodThread, consThreads[consumerNum];

    struct timeval startTime, endTime;
    gettimeofday(&startTime, NULL);

    pthread_create(&prodThread, NULL, manager, (void *)args);

    for (int i = 0; i < consumerNum; i++)
        pthread_create(&consThreads[i], NULL, worker, NULL);

    pthread_join(prodThread, NULL);
    for (int i = 0; i < consumerNum; i++)
        pthread_join(consThreads[i], NULL);

    gettimeofday(&endTime, NULL);
    double elapsedSeconds = endTime.tv_sec - startTime.tv_sec;
    double elapsedMicroseconds = endTime.tv_usec - startTime.tv_usec;
    double totalElapsedTime = elapsedSeconds + elapsedMicroseconds / 1000000.0;

    printf("\nTotal elapsed time: %.4f seconds\n", totalElapsedTime);
    printf("Total bytes copied: %lld bytes\n", totalByte);    // each directory is 4 kbytes
    printf("Total files copied number: %ld \n", regularFileNum + directoryNum + FIFONUM);
    printf("Copied 'directory' number: %ld\n", directoryNum);
    printf("Copied 'regular file' number: %ld\n", regularFileNum);
    printf("Copied 'FIFO' number: %ld\n", FIFONUM);

    cleanup(); // Free allocated resources

    return 0;
}
