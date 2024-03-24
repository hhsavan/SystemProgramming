#include "helper.h"

// this function will log the given parameters to a file
/*
    childOrParent: parent (main) process=0 - child(worker)=1
    processPid: process id of caller function
    functionName: Hangi Fonksiyon içinden çağırıldı
    workingFile: Şuan hangi dosya üzerinde çalışıyor
    startOrEndtime: 0: startTime of the function - 1: endTime of the caller function
*/
void Log(int childOrParent, const int processPid, const char *functionName, const char *workingFile, int startOrEndtime)
{

    char *filename = "LOG";
    int fd;

    // Open the file for writing with append mode
    fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (fd == -1)
    {
        perror("open");
        exit(EXIT_FAILURE);
    }

    struct flock lock;
    lock.l_type = F_WRLCK;
    fcntl(fd, F_SETLKW, &lock);

    // Locked the file.
    // Get current date and time
    time_t now;
    time(&now);
    char formattedTime[80]; // Define a buffer for formatted time string
    strftime(formattedTime, sizeof(formattedTime), "%Y-%m-%d %H:%M:%S", localtime(&now));

    // Create the log message dynamically
    int message_len = strlen(functionName) + 3 + strlen(workingFile) + strlen(formattedTime) + 50; // Estimate message length
    char *message = (char *)malloc(message_len);                                                   // Allocate memory for the message
    if (message == NULL)
    {
        perror("malloc");
        close(fd);
        exit(EXIT_FAILURE);
    }
    char tempstr[35];
    char *whiteSpaceStr = " - ";

    // Formating
    strcpy(message, formattedTime);
    // printf("functionName: %s\n", functionName);
    sprintf(tempstr, "%d", processPid);
    strcat(message, whiteSpaceStr);
    strcat(message, tempstr);
    strcat(message, whiteSpaceStr);
    if (childOrParent == 1)
        strcat(message, "ChildProcess");
    else
        strcat(message, "MainProcess");
    strcat(message, whiteSpaceStr);
    strcat(message, functionName);
    strcat(message, whiteSpaceStr);
    strcat(message, workingFile);
    strcat(message, whiteSpaceStr);
    if (startOrEndtime == 0)
        strcat(message, "started");
    else
        strcat(message, "end");
    strcat(message, "\n");

    if (write(fd, message, strlen(message)) == -1)
    {
        perror("write");
        close(fd);
        free(message); // Free memory even on error
        exit(EXIT_FAILURE);
    }
    // Release the lock.
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLKW, &lock);
    // Free the allocated memory
    free(message);

    // Close the log file
    close(fd);
}

void LogErr(char *errorMessage, int childOrParent, const int processPid, const char *functionName)
{
    char *filename = "LOG";
    int fd;

    // Open the file for writing with append mode
    fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (fd == -1)
    {
        perror("open");
        exit(EXIT_FAILURE);
    }
    struct flock lock;
    lock.l_type = F_WRLCK;
    fcntl(fd, F_SETLKW, &lock);
    // Get current date and time
    time_t now;
    time(&now);
    char formattedTime[80]; // Define a buffer for formatted time string
    strftime(formattedTime, sizeof(formattedTime), "%Y-%m-%d %H:%M:%S", localtime(&now));

    // Create the log message dynamically
    int message_len = strlen(functionName) + 3 + strlen(formattedTime) + 50; // Estimate message length
    char *message = (char *)malloc(message_len);                             // Allocate memory for the message
    if (message == NULL)
    {
        perror("malloc");
        close(fd);
        exit(EXIT_FAILURE);
    }
    char tempstr[35];
    char *whiteSpaceStr = " - ";

    // Formating
    strcpy(message, formattedTime);
    // printf("functionName: %s\n", functionName);
    sprintf(tempstr, "%d", processPid);
    strcat(message, whiteSpaceStr);
    strcat(message, tempstr);
    strcat(message, whiteSpaceStr);
    if (childOrParent == 1)
        strcat(message, "ChildProcess");
    else
        strcat(message, "MainProcess");
    strcat(message, whiteSpaceStr);
    strcat(message, functionName);
    strcat(message, whiteSpaceStr);
    strcat(message, " ! ERROR: ");
    strcat(message, errorMessage);
    strcat(message, "\n");

    if (write(fd, message, strlen(message)) == -1)
    {
        perror("write");
        close(fd);
        free(message); // Free memory even on error
        exit(EXIT_FAILURE);
    }

    // Release the lock.
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLKW, &lock);
    // Free the allocated memory
    free(message);

    // Close the log file
    close(fd);
}

// CommandArr yapısı:
/*
    0: komut tipi
    1: isim soyisim
    2: grade
    3: dosya
    4: numOfEntries (for listSome)
    5: PageNumber (for listSome)
*/
// buraya child processler gelir
int CommandManager(char *command)
{
    // printf("ChildProcess id: %d\n", getpid());
    if (command == NULL)
    {
        LogErr("InvalidCommand", 1, getpid(), "CommandManager");

        return -1; // error
    }
    char **commandArr = StringParser(command);
    char *cm0 = LowerCase(commandArr[0]);
    // printf("asdasdsadadasd:%s\n:",cm0);
    // printf("asdasd : %s\n",cm0);
    int id = fork();
    if (!strcmp("gtustudentgrades", cm0))
    {
        if (id == 0)
            Log(1, getpid(), "GtuStudentGrades", commandArr[1], 0);
        else
        {
            GtuStudentGrades(commandArr[1]);
            Log(1, getpid(), "GtuStudentGrades", commandArr[1], 1);
        }
    }
    else if (!strcmp("addstudentgrade", cm0))
    {
        if (id == 0)
            Log(1, getpid(), "AddStudentGrade", commandArr[3], 1);
        else
        {
            AddStudentGrade(commandArr[1], commandArr[2], commandArr[3]);
            Log(1, getpid(), "AddStudentGrade", commandArr[3], 0);
        }
    }
    else if (!strcmp("searchstudent", cm0))
    {
        if (id == 0)
            Log(1, getpid(), "SearchStudent", commandArr[2], 1);
        else
        {
            SearchStudent(commandArr[1], commandArr[2]);
            Log(1, getpid(), "SearchStudent", commandArr[2], 0);
        }
    }
    else if (!strcmp("showall", cm0))
        if (id == 0)
            Log(1, getpid(), "ShowAll", commandArr[1], 1);
        else
        {
            ShowAll(commandArr[1]);
            Log(1, getpid(), "ShowAll", commandArr[1], 0);
        }
    else if (!strcmp("listgrades", cm0))
        if (id == 0)
            Log(1, getpid(), "listgrades", commandArr[1], 1);
        else
        {
            List(commandArr[1], 0, 5);
            Log(1, getpid(), "listgrades", commandArr[1], 0);
        }
    else if (!strcmp("listsome", cm0))
    {

        if (id == 0)
            Log(1, getpid(), "listsome", commandArr[3], 1);
        else
        {
            // printf("1.%c\n",commandArr[1][0]);//5
            // printf("2.%c\n",commandArr[1][2]);//5
            // printf("3.%s\n",commandArr[2]);//5
            char *cp1 = &commandArr[1][0];
            char *cp2 = &commandArr[1][2];
            int start = atoi(cp1) * (atoi(cp2) - 1);
            int end = atoi(cp1) * atoi(cp2);
            // printf("\nstart: %d , end: %d\n",start,end);
            List(commandArr[2], start,end);
            Log(1, getpid(), "listsome", commandArr[2], 0);
        }
    }
    else if (!strcmp("sortall", cm0))
        if (id == 0)
            Log(1, getpid(), "sortall", commandArr[3], 1);
        else
        {
            SortAll();
            Log(1, getpid(), "ShowAll", commandArr[3], 0);
        }
    else if (!strcmp("usage\n", command))
        if (id == 0)
            Log(1, getpid(), "usage", commandArr[3], 1);
        else
        {
            Usage();
            Log(1, getpid(), "Usage", commandArr[3], 0);
        }
    else
    {
        LogErr("InvalidCommand", 1, getpid(), "CommandManager");
        print("\n#COMMAND ERROR#\n");
        // exit(EXIT_FAILURE);
    }
    wait(NULL); // child processin bitmesini bekle böylece  zombie olmaz
    free2d((void **)commandArr);
    exit(EXIT_SUCCESS);
    return 0;
}

void GtuStudentGrades(const char *fileName)
{

    int fd = open(fileName, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (fd == -1)
    {
        perror("Error while opennig the file");
        LogErr("Error while opennig the file", 1, getpid(), "GtuStudentGrades");
        return;
    }
    else{
        print("File is Opend:");
        print(fileName);
        print("\n");
    }
}

char *LowerCase(char *str)
{
    if (str == NULL)
    {
        return NULL; // fd NULL input
    }

    int i = 0;
    while (str[i] != '\0')
    {
        str[i] = tolower(str[i]); // Convert each character to lowercase
        i++;
    }

    return str; // Return the modified string
}
// verilen stringi parçalara ayırır ve char** olarak return eder:
// addStudentGrade "Name Surname" "AA" "grades.txt"
//  boşluklara göre ayırsın
char **StringParser(char *strP)
{
    // Error handling for invalid input
    if (strP == NULL)
    {
        perror("Error: Input string is NULL.\n");
        return NULL;
    }

    char *delim = " "; // Use space as delimiter

    int i = 0, numTokens = 10; // aslnda maks 6
    char **parsedWords = (char **)malloc(numTokens * sizeof(char *));
    if (parsedWords == NULL)
    {
        perror("Memory allocation failed.\n");
        LogErr("Memory allocation failed.", 1, getpid(), "StringParser");
        return NULL;
    }
    char *token = strtok(strP, delim);
    delim = "\"";
    // numTokens++;
    while (token != NULL)
    {
        // aradaki boşlukları alma
        if (strlen(token) > 1)
        {
            // numTokens++;
            parsedWords[i] = strdup(token);
            i++;
        }
        token = strtok(NULL, delim);
    }
    // free()
    return parsedWords; // Return the parsed array
}

// burda search operasyonu da olacak. dosyaya eklemeden önce aynı isimde birinin oluğ olmadığına bakacak
//  ama aynı isimde ve notta iki kişi olabilir mesela ahmet yıldız çok kullanılan bir isim
//  son satırı tutacak buraya kadar okunabilir ya da yazılabilir. ???
int AddStudentGrade(const char *nameP, const char *gradeP, const char *fileName)
{
    // printf("AddStudentGrade Fonksiyonu:%d \n", getpid());
    // aynı isimde öğrenci var
    // if (SearchStudent(nameP, fileName) != NULL){

    //     perror("The Student already exist");
    //     return -1; // nuraya structtan değer koyacam (DUPLICATE_VAL_ERR)
    // }

    int fd; // File descriptor

    // Open a file for writing
    fd = open(fileName, O_WRONLY | O_APPEND, 0666);

    if (fd == -1)
    {
        perror("Error opening file");
        LogErr("Error opening file", 1, getpid(), "GtuStudentGrades");
        exit(EXIT_FAILURE);
    }

    // start locking the file
    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_start = 0;
    lock.l_whence = SEEK_SET;
    lock.l_len = 0;
    // Data to write to the file
    char comma[] = ", ";
    char newline[] = "\n";
    size_t len = strlen(nameP) + strlen(gradeP) + strlen(comma) + strlen(newline);

    char *newcpt = (char *)malloc(len + 1); // +1 for null terminator

    if (newcpt == NULL)
    {
        perror("Memory allocation failed");
        LogErr("Memory allocation failed", 1, getpid(), "AddStudentGrade");

        close(fd);
        exit(EXIT_FAILURE);
    }

    strcpy(newcpt, nameP);
    strcat(newcpt, comma);
    strcat(newcpt, gradeP);
    strcat(newcpt, newline);

    // LOCKED
    if (fcntl(fd, F_SETLKW, &lock) == -1)
    {
        if (errno == EINTR)
        {
            perror("Lock already acquired by another process.\n");
        }
        else
        {
            perror("fcntl");
        }
    }
    else
    {
        // Critical section: Write to the file
        // printf("Process (PID: %d) writing to file...\n", getpid());
        ssize_t bytes_written = write(fd, newcpt, strlen(newcpt)); // Use strlen(newcpt) for the size

        lock.l_type = F_UNLCK; // Unlock
        // if (fcntl(fd, F_SETLK, &lock) == -1)
        // {
        //     perror("fcntl");
        // }
        if (bytes_written == -1)
        {
            perror("Error writing to file");
            LogErr("Error writing to file", 1, getpid(), "AddStudentGrade");
            close(fd);
            free(newcpt);
            exit(EXIT_FAILURE);
        }
    }

    // Close the file
    if (close(fd) == -1)
    {
        perror("Error closing file");
        LogErr("Error closing to file", 1, getpid(), "AddStudentGrade");
        free(newcpt);
        exit(EXIT_FAILURE);
    }
    // print("Students Added: ");
    // printf("Students Added: %d\n", getpid());
    free(newcpt);
    return 0;
}

void SearchStudent(const char *nameP, const char *fileName)
{
    printf("FileName: %s\n", fileName);
    int fd = open(fileName, O_RDONLY, 0666);
    if (fd == -1)
    {
        perror("File open error");
        exit(EXIT_FAILURE);
    }

    char buffer[1024];
    char *newline;
    long bytesRead;
    int isFound = 0;
    char *chptr;
    struct flock lock;
    lock.l_type = F_RDLCK;
    fcntl(fd, F_SETLKW, &lock);
    while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0 && isFound == 0)
    {
        chptr = buffer;
        while ((newline = strchr(chptr, '\n')) != NULL)
        {
            *newline = '\0';
            char *temp = strdup(chptr);
            char *token = strtok(temp, ",");
            if (token != NULL && strcmp(token, nameP) == 0)
            {
                printf("\nStudent found: %s\n", chptr);
                isFound = 1;
                break;
            }
            chptr = newline + 1;
        }
    }

    if (isFound == 0)
    {
        print("\nStudent not found.\n");
    }

    // Release the lock.
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLKW, &lock);

    close(fd);
}

int ShowAll(const char *fileName)
{
    int fd = open(fileName, O_RDONLY);
    if (fd == -1)
    {
        perror("Error opening file");
        return -1; // Dosya açılamadı, hata kodu olarak -1 dön
    }

    // Dosyayı kilitler
    if (flock(fd, LOCK_SH) == -1)
    {
        perror("Error locking file");
        close(fd);
        return -1; // Dosya kilitlenemedi, hata kodu olarak -1 dön
    }

    struct flock lock;
    lock.l_type = F_WRLCK;
    fcntl(fd, F_SETLKW, &lock);
    char buffer[1024];
    ssize_t bytesRead;
    while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0)
    {
        if (write(STDOUT_FILENO, buffer, bytesRead) == -1)
        {
            perror("Error writing to stdout");
            close(fd);
            return -1; // Standart çıktıya yazılamadı, hata kodu olarak -1 dön
        }
    }
    // Release the lock
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLKW, &lock);

    if (bytesRead == -1)
    {
        perror("Error reading file");
        close(fd);
        return -1; // Dosyadan okuma hatası, hata kodu olarak -1 dön
    }

    // Dosya kilidini kaldırır
    if (flock(fd, LOCK_UN) == -1)
    {
        perror("Error unlocking file");
        close(fd);
        return -1; // Dosya kilidi kaldırılamadı, hata kodu olarak -1 dön
    }

    close(fd); // Dosyayı kapat
    return 0;  // Başarılı, 0 dön
}

void List(const char *fileName, int start, int end)
{
    int fd;
    fd = open(fileName, O_RDONLY, 0666); // Open for reading
    if (fd == -1)
    {
        perror("Failed to obtain fd"); // Use "fd" instead of "fd"
        exit(EXIT_FAILURE);
    }

    // Seek to the beginning (alternative to lseek)
    off_t offset = lseek(fd, 0, SEEK_SET);
    if (offset == -1)
    {
        perror("Failed to seek");
        close(fd);
        exit(EXIT_FAILURE);
    }

    char buffer[1024];
    ssize_t bytes_read;
    char *delim;
    int line_count = 0;

    struct flock lock_data;          // Use a different struct name
    lock_data.l_type = F_RDLCK;      // Read lock
    fcntl(fd, F_SETLKW, &lock_data); // Lock the file

    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0)
    {
        char *last_delim = strrchr(buffer, '\n'); // Find last newline

        // fd incomplete line (similar logic)
        if (last_delim != NULL)
        {
            offset = lseek(fd, last_delim - buffer + 1 - bytes_read, SEEK_CUR);
            if (offset == -1)
            {
                perror("Failed to seek");
                close(fd);
                exit(EXIT_FAILURE);
            }
            *(last_delim + 1) = '\0'; // Terminate the line
        }

        char *ptr = buffer; // Pointer to current position
        while ((delim = strchr(ptr, '\n')) != NULL)
        {
            *delim = '\0'; // Replace newline with null terminator

            // Display line if within range (similar logic)
            if (end == -1 || (line_count >= start && line_count < end))
            {
                write(STDOUT_FILENO, ptr, strlen(ptr)); // Use write instead of printf
                write(STDOUT_FILENO, "\n", 1);          // Add newline
            }

            ptr = delim + 1; // Move to the next line
            line_count++;
        }
    }

    // Unlock the file
    lock_data.l_type = F_UNLCK;
    fcntl(fd, F_SETLKW, &lock_data);

    close(fd);
}

void Usage()
{

    printf("\nGTU Student Grade Management System\n");
    printf("Available commands:\n\n");

    // gtuStudentGrades
    printf("  * `gtuStudentGrades <\"fileName\">`\n");
    printf("      - Creates a new file for student grades.\n");

    // addStudentGrade
    printf("  * `addStudentGrade <\"Name Surname\"> <\"Grade\"> <\"fileName\">`\n");
    printf("      - Adds a new student's name, grade, and saves it to the specified file.\n");

    // searchStudent
    printf("  * `searchStudent <\"Name Surname\"> <\"fileName\">`\n");
    printf("      - Searches for a student's grade by their full name in the specified file.\n");

    // sortAll
    printf("  * `sortAll <\"sortBy\"> <\"sortOrder\"> <\"fileName\">`\n");
    printf("      - Sorts all entries in the file based on a field (name or grade).\n");
    printf("        - `sortBy`: `0` for name, `1` for grade.\n");
    printf("        - `sortOrder`: `0` for ascending order, `1` for descending order.\n");

    // showAll
    printf("  * `showAll <\"fileName\">`\n");
    printf("      - Displays all student entries stored in the specified file.\n");

    // listGrades
    printf("  * `listGrades <\"fileName\">`\n");
    printf("      - Displays the first 5 student entries from the specified file.\n");

    // listSome
    printf("  * `listSome <numOfEntries> <pageNumber> <\"fileName\">`\n");
    printf("      - Displays a specific number of entries on a particular page from the file.\n");
    printf("        - `numOfEntries`: Number of entries per page.\n");
    printf("        - `pageNumber`: Page number to display (starts from 1).\n");

    printf("\n");
    fflush(stdout);
}
int SortAll()
{
    printf("\nNOT IMPLEMENTED\n\n");

    return -1;
}

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

void print2d2(char **vp)
{
    if (vp == NULL)
    {
        return;
    }

    int i = 0;
    while (vp[i] != NULL)
    {
        // Print the current string
        write(STDOUT_FILENO, vp[i], strlen(vp[i]));
        write(STDOUT_FILENO, "\n", 1); // Add a newline character

        i++;
    }
}

size_t GetInput(char *buffer, size_t bufferSize)
{

    size_t bytesRead = read(0, buffer, BUFFER_SIZE - 1); // 0 indicates stdin

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
int print2d(char **strP)
{
    if (strP == NULL)
    {
        write(STDOUT_FILENO, "Error: Input array is NULL.\n", sizeof("Error: Input array is NULL.\n"));
        return -1;
    }

    int i = 0;
    while (strP[i] != NULL)
    {
        write(STDOUT_FILENO, strP[i], strlen(strP[i])); // Write entire string at once
        write(STDOUT_FILENO, "\n", 1);                  // Add a newline
        i++;
    }

    return 0;
}

int printStruct(EntryStruct **strP, int start, int last)
{
    if (strP == NULL)
    {
        return -1;
    }

    for (int i = start; i <= last; i++)
    {
        write(STDOUT_FILENO, "name: ", sizeof("name: "));
        write(STDOUT_FILENO, strP[i]->name, strlen(strP[i]->name)); // Write name
        write(STDOUT_FILENO, "\n", 1);

        write(STDOUT_FILENO, "grade: ", sizeof("grade: "));
        write(STDOUT_FILENO, strP[i]->grade, strlen(strP[i]->grade)); // Write grade
        write(STDOUT_FILENO, "\n", 1);
    }

    return 0;
}
