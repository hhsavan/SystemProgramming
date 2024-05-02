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



#define MAX_CLIENTS 2
#define MAX_BUF_SIZE 256
#define SERVER_FIFO "serverFifo"
char* clientFifo; // cl_pid >> cl_1256
sig_atomic_t* childPids;   //clientlerle ilgilenen child processler
sig_atomic_t* connectedPids;  //server'a bağlanan processler
sig_atomic_t* connectedClientN;
// #define CL_FIFO strcat(clgetpid());
enum connectionStatus{
    connect,
    tryConnect,
    established,
    notEstablished,
    serverShutdown,
    clientShutdown

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
typedef enum request_type {
    LIST,
    READF,
    WRITET,
    UPLOAD,
    DOWNLOAD,
    QUIT,
    KILL
}request_type;

typedef struct Request {
    request_type request;
    char filename[256];
    u_int64_t offset;
    char string[4096];
    char client_dir[256];

} Request;

typedef struct Message Message;
typedef struct ConnectionReq ConnectionReq;

void sendKillSig(pid_t pid);
#endif
