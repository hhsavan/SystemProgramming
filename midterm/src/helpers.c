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
void free1d(void *vp)
{
    if (vp == NULL)
        return;
    free(vp);
}

void free2d(void **vp)
{
    if (vp == NULL)
        return;

    // Assuming vp points to an array of pointers:
    int i = 0;
    while (vp[i] != NULL)
    {
        free(vp[i]); // Free each string allocated by strdup
        i++;
    }

    free(vp); // Free the top-level array allocated with malloc
}

size_t GetInput(char *buffer, size_t bufferSize)
{

    size_t bytesRead = read(0, buffer, bufferSize - 1); // 0 indicates stdin

    if (bytesRead == -1)
        return 1;

    buffer[bytesRead] = '\0';
    return bytesRead;
}

int print(const char *strP)
{
    if (strP == NULL)
    {
        return -1;
    }

    int i = 0;
    while (strP[i] != '\0')
    {
        write(STDOUT_FILENO, &strP[i], 1); // Write each character individually
        i++;
    }

    return 0;
}