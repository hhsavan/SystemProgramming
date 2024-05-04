// #ifndef FIFO_SEQNUM_H_INCLUDED
// #define FIFO_SEQNUM_H_INCLUDED

#pragma

#include <sys/types.h> 
#include <sys/stat.h> 
#include <fcntl.h>

#define SEMAPHORE_NAME "/seqnum_semaphore"
sem_t *sem; 
#define TRY_CONNECT 0
#define CONNECT 1

#define FAILURE 0
#define SUCCESS 1
#define DISCONNECT 2
#define WAIT 3

# define MAX_TOKENS 4
# define MAX_WORD_SIZE 250

#define SERVER_FIFO "/tmp/seqnum_sv"

#define CLIENT_FIFO_TEMPLATE "/tmp/seqnum_cl.%ld"

#define REQUEST_CLIENT_FIFO_TEMPLATE "/tmp/seqnum_cl_req.%ld"

#define CLIENT_FIFO_NAME_LEN (sizeof(CLIENT_FIFO_TEMPLATE) + 20)
// enum ConnectionStatus{
// 	serverIsFull_exit,	//server is full
// 	serverIsFull_wait,	//server is full
// 	connected
// };

// struct ClientInfo *connectedClients;
struct request { 
	pid_t pid;   
	int connectOption; 
	int seqLen;  
	char comment[500]; 
};

struct response { 
	int seqNum;	  
	int serverConnection;
	// enum ConnectionStatus serverConnection;
	int serverPid; 
	char commentResult[500];
};

struct ClientInfo {
    int clientpid;   // Client PID
    int childpid;    // Child PID
    int flag;        // Flag: 1 means working, 0 means not working
};
void closew(int fd);
void printReq(const struct request* str);
void printRes(const struct response* str);
void sendRequst(const char* fifo, const struct request* req);
void sendThatConnectionEstablishedtoClient(const char* clientFifo);

// #endif