#ifndef __HELPER_H__
#define __HELPER_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>



#define MAX_CLIENTS 10
#define MAX_BUF_SIZE 256
#define SERVER_FIFO "serverFifo"
char* clientFifo; // cl_pid >> cl_1256
// #define CL_FIFO strcat(clgetpid());
enum connectionStatus{
    connect,
    tryConnect,
    established,
    notEstablished

}connectionStatus;

struct ConnectionReq{
    pid_t pid;
    enum connectionStatus type;
};


struct Message{
    char* msg;
    int length;
    pid_t sender;   //her aşamada bu kontrol edilecek. araya biri girmiş mi diye
};

typedef struct Message Message;
typedef struct ConnectionReq ConnectionReq;


#endif
