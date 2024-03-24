#include "helper.h"


char buffer[BUFFER_SIZE];
ssize_t bytes_read;

// w-w lock tmm
// TODO yazar blocklarken okuma yapılabiliyor mu buna bakılacak!!!
int main(int argc, char *argv[])
{

    printf("Main Process id: %d\n", getpid());
    // ListGrades("grades.txt");

    // List("grades.txt",5,2);
    while(1){
        // printf("\n>: ");
        fflush(stdout);
        bytes_read = GetInput(buffer, BUFFER_SIZE);

        if (bytes_read == -1) {
            perror("Error reading input");
            return 1;
        }

        int id = fork();
        if(id==-1){
            perror("Fork yapılamadı- child process oluşturulamadı");
            continue;
        }
        if (id==0) //child process
        {
            // printf("buf:%s\n",buffer);
            // fflush(stdout);
            CommandManager(buffer);
            // // AddStudentGrade("asdasdasd", "AA", "grades.txt");

        }

        // Print the input read from the user
        // printf("Input read from user: %s\n", buffer);
        // break;
    }
    // END of the main process
    Log(0,getpid(),"main","X",1);
    return 0;
}