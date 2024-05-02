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


void printReq(const struct request* str){
    printf("pid: %d\n",str->pid);
    printf("seqLen: %d\n",str->seqLen);
    printf("comment: %s\n",str->comment);
    printf("connectOption: %d\n",str->connectOption);
}
void printRes(const struct response* str){
    // printf("pid: %ld\n",str->seqNum);
    // printf("seqLen: %d\n",str->seqLen);
    // printf("comment: %s\n",str->comment);
    // printf("connectOption: %s\n",str->connectOption);
}