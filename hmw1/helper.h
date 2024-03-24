#ifndef __HELPER_H_
#define __HELPER_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h> 
#include <string.h>
#include <sys/file.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>

#define BUFFER_SIZE 1024
#define MAX_LINE_LENGTH 256
#define NUM_ENTRIES 5 //listGrade fonksionu için



typedef struct EntryStruct
{
    char* name;
    char* grade;

}EntryStruct;

// char* commandsArr;

// CommandStruct cmdStr;
size_t GetInput(char* buffer, size_t bufferSize);

extern char buffer[BUFFER_SIZE];
extern ssize_t bytes_read;
void print2d2(char **vp);
void free1d(void* vp);
void free2d(void** vp);


void Log(int childOrParent, const int processPid, const char* functionName, const char* workingFile, int startOrEndtime);
void LogErr(char *errorMessage, int childOrParent, const int processPid, const char *functionName);

void GtuStudentGrades();
char** StringParser(char* strP);

int CommandManager(char* command);


int AddStudentGrade(const char* nameP, const char* gradeP, const char* fileName);

void SearchStudent(const char *nameP, const char *fileName);

int ShowAll(const char* fileName);

int ListGrades(const char* fileName);

void List(const char *fileName, int start, int end);


//ÇALIŞMIYOR
int SortAll();

void Usage();

int print(const char* strP);
int print2d(char** strP);
char* LowerCase(char* str);
int printStruct(EntryStruct **strP, int start, int last);




#endif