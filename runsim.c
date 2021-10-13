//runsim.c

#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <signal.h>
#include "detachandremove.h"
#include "license.h"
#define PERM (IPC_CREAT | S_IRUSR | S_IWUSR)

#define MAX_CANON 1024

// FUNCTION PROTOTYPES
void initShm();                                     //This function will initialize shared memory.
void initSem();                                     //This is going to initialize our semaphores.
int removeSem();                                    //This is going to remove our semaphores.
void semWait();                                     //This is for our semaphore wait
void semSignal();                                   //This is for our semaphore signals
void setsembuf(struct sembuf *s, int num, int op, int flg);
int r_semop(int semid, struct sembuf *sops, int nsops);
void docommand();                      //This function will be doing the exec functions.
void createChildren();                              //This function will create the children processes from main process
void parsingArgs(int argc, char** argv);            //This helper function will parse command line args.

// SIGNAL HANDLERS
static void myTimeOutHandler( int s );                    //This is our signal handler for timeouts
static void myKillSignalHandler( int s );                 //This is our signal handler for interrupts
static int setupUserInterrupt( void );
static int setupinterrupt( void );
static int setupitimer( int tValue );

/* THINGS TO DO (Project3):
 * CLEAN THIS UP. I WANNA MOVE ALL THE ESSENTIAL STUFF INTO ORGANIZED FILES SO MAIN FILE DOESN'T LOOK SO CLOGGED.
 */

// GLOBALS
enum state{idle, want_in, in_cs};
int opt, timer, nValue, tValue;                     //This is for managing our getopts
int currentConcurrentProcesses = 1;                 //Initialized as 1 since the main program is also a process.
int totalProcessesCreated = 0;                      //number of created process
int shmID, waitStatus;                       //This is for managing our processes
key_t myKey;                                        //Shared memory key
sharedMem *sharedHeap;                              //shared memory object
int semID;                                          //SEMAPHORE ID
struct sembuf semW;                              //SEMAPHORE WAIT
struct sembuf semS;                              //SEMAPHORE SIGNAL
char execArgs[3][MAX_CANON];                        //the execl arguments for the grandchild processes

int main( int argc, char* argv[]){
    //gonna make a signal interrupt here just to see what happens
    if( setupUserInterrupt() == -1 ){
        perror( "failed to set up a user kill signal.\n");
        return 1;
    }

    char *tempStr;
    char buffer[MAX_CANON];
    int i, currentIndex, count = 0;

    // PARSING ARGUMENTS FIRST
    parsingArgs(argc, argv);

    //setting up interrupts after parsing arguments
    if (setupinterrupt() == -1) {
        perror("Failed to set up handler for SIGALRM");
        return 1;
    }

    if (setupitimer(tValue) == -1) {
        perror("Failed to set up the ITIMER_PROF interval timer");
        return 1;
    }

    // Parsing is finished, now we are allocating and adding to licenses
    initShm();
    initSem();
    sharedHeap->nlicense = initlicense(sharedHeap);
    addtolicenses(sharedHeap, nValue);

    //***************************** LOOP SECTION FOR CREATING NEW PROCESSES ******************************
    do{
        //We'll be using fgets() for our stdin. "testing.data" is what we will be receiving from.
        fflush(stdin);
        fgets(buffer, MAX_CANON, stdin);
        printf("strlen: %ld\n", strlen(buffer));
        printf("buffer: %s\n", buffer);
        size_t lineNum = strlen(buffer) - 1;
        tempStr = strtok(buffer, " ");
        count = 0;
        while (tempStr != NULL)
        {
            //printf ("tempStr: %s\n",tempStr);
            strcpy(execArgs[count], tempStr);
            tempStr = strtok (NULL, " ");
            //printf("execArgs[%d]: %s\n", count, execArgs[count]);
            count++;
        }

        // clearing out buffer.
        if(buffer[lineNum] == '\n')
            buffer[lineNum] = '\0';

        //creating child processes. We'll only create children as long as we have arguments loaded from testing.data!
        createChildren();
    }while(!feof(stdin));

    // Getting a new line after reading file
    printf("END OF STDIN\n");

    //***************************** LOOP SECTION FOR CREATING NEW PROCESSES ******************************

    /* the parent process */
    // waiting for the child process
    //waiting for max amount of children to terminate
    for (i = 0; i <= 20 ; ++i) {
        /* the parent process */
        if (wait(&waitStatus) == -1) {
            perror("Failed to wait for child\n");
        } else {
            if (WIFEXITED(waitStatus)) {
                currentConcurrentProcesses--;
                printf("current concurrent process %d\n", currentConcurrentProcesses);
                printf("Child process successfully exited with status: %d\n", waitStatus);
                printf("Total number of licenses %d\n", sharedHeap->nlicense);
                printf("total processes created: %d\n", totalProcessesCreated);
            }
        }
    }

    //detaching shared memory
    if(detachandremove(shmID, sharedHeap) == -1){
        perror("Failed to destroy shared memory segment");
        exit(EXIT_FAILURE);
    } else {
        printf("Memory segment detached!\n");
    }

    //removing semaphores
    if(removeSem(semID) == -1){
        perror("Failed to remove semaphore");
        exit(EXIT_FAILURE);
    }

    return 0;
}

void initShm(){
    //********************* SHARED MEMORY PORTION ************************

    if((myKey = ftok(".",1)) == (key_t)-1){
        //if we fail to get our key.
        fprintf(stderr, "Failed to derive key from filename:\n");
        exit(EXIT_FAILURE);
    }
    printf("derived key from, myKey: %d\n", myKey);

    if( (shmID = shmget(myKey, sizeof(sharedMem), PERM)) == -1){
        perror("Failed to create shared memory segment\n");
        exit(EXIT_FAILURE);
    } else {
        // created shared memory segment!
        printf("created shared memory!\n");

        if ((sharedHeap = (sharedMem *) shmat(shmID, NULL, 0)) == (void *) -1) {
            perror("Failed to attach shared memory segment\n");
            if (shmctl(shmID, IPC_RMID, NULL) == -1) {
                perror("Failed to remove memory segment\n");
            }
            exit(EXIT_FAILURE);
        }
        // attached shared memory
        printf("attached shared memory\n");
    }
    //****************** END SHARED MEMORY PORTION ***********************
}

void initSem(){
    if( (semID = semget(myKey, 1, PERM | IPC_CREAT)) == -1){
        perror("Failed to create semaphore with key\n");
        exit(EXIT_FAILURE);
    } else {
        printf("Semaphore created with key\n");
    }
    //setsembuf(&semW, 0, -1, 0);
    //setsembuf(&semS, 0, 1, 0);
}

int removeSem(){
    return semctl(semID, 0, IPC_RMID);
}


void semWait(){
    if (semop(semID, &semW, 1) == -1) {
        perror("semop");
        exit(1);
    }
}

void semSignal(){
    if (semop(semID, &semS, 1) == -1) {
        perror("semop");
        exit(1);
    }
}

int r_semop(int semid, struct sembuf *sops, int nsops){
    while(semop(semid, sops, nsops) == -1){
        if(errno != EINTR){
            return -1;
        }
    }
    return 0;
}

void setsembuf(struct sembuf *s, int num, int op, int flg){
    s->sem_num = (short)num;
    s->sem_op = (short)op;
    s->sem_flg = (short)flg;
    return;
}

void docommand(){
    //This function will use getlicense() to gather an idea of how many nlicenses are around.
    //If there are 0 then we be blocking until it is ok to receive a license.

    /* Usage: ./testsim [-s seconds for sleep] [-r number of repeats]
     * We'll need to set up just like this
    */

    pid_t gchildPid;
    int error;
    //This means we've run out of license and must wait for more
    do {

    }while( getlicense(sharedHeap) == -1);
    //*********************************** ENTRY SECTION **********************************************
    semWait();
    //********************************** CRITICAL SECTION ******************************************
    if (currentConcurrentProcesses <= MAX_PROC / 2) {
        if ((gchildPid = fork()) == -1) {
            perror("Failed to create grandchild process\n");
            if (detachandremove(shmID, sharedHeap) == -1) {
                perror("Failed to destroy shared memory segment");
            }
            exit(EXIT_FAILURE);
        }
        currentConcurrentProcesses++;
        totalProcessesCreated++;

        // made a grandchild process!
        if (gchildPid == 0) {
            /* the grandchild process */
            //debugging output
            printf("current concurrent process %d: myPID: %ld\n", currentConcurrentProcesses, (long) getpid());
            sleep(1);
            execl("./testsim", execArgs[0], execArgs[1], execArgs[2], NULL);
        } else {
            /* the parent of grandchild process */
            if (wait(&waitStatus) == -1) {
                perror("Failed to wait for grandchild\n");
            } else {
                if (WIFEXITED(waitStatus)) {
                    currentConcurrentProcesses--;
                    returnlicense(sharedHeap);
                    printf("current concurrent process %d\n", currentConcurrentProcesses);
                    printf("Child process successfully exited with status: %d\n", waitStatus);
                    printf("Total number of licenses %d\n", sharedHeap->nlicense);
                    printf("total processes created: %d\n", totalProcessesCreated);
                    //********************************** EXIT SECTION **********************************************
                    semSignal();
                    //******************************** REMAINDER SECTION *******************************************
                    exit(EXIT_SUCCESS);
                }
            }
        }

    }
}

void createChildren() {
    int myID = 0;
    pid_t childPid;
    if (currentConcurrentProcesses <= MAX_PROC / 2) {
        //changed this from while to if, This way it will only create 1 child!
        if ((childPid = fork()) == -1) {
            perror("Failed to create child process\n");
            if (detachandremove(shmID, sharedHeap) == -1) {
                perror("Failed to destroy shared memory segment");
            }
            exit(EXIT_FAILURE);
        }

        currentConcurrentProcesses++;
        totalProcessesCreated++;

        // made a child process!
        if (childPid == 0) {
            /* the child process */
            myID = totalProcessesCreated;                               //assuming at max I will create 20 procs for now

            //debugging output
            printf("current concurrent process %d: myPID: %ld\n", currentConcurrentProcesses, (long) getpid());

            //calling the docommand to handle the licensing.

            //We've got a fork that will be making an execl
            docommand();
            exit(EXIT_SUCCESS);
        }
    }
}

void parsingArgs(int argc, char** argv){
    while((opt = getopt(argc, argv, "ht:")) != -1) {
        switch (opt) {
            case 'h':
                //This is the help parameter. We'll be printing out what this program does and will end the program.
                //If this is entered along with others, we'll ignore the rest of the other parameters to print help
                //and end the program accordingly.
                printf("Usage: %s [-h] [-t <seconds. DEFAULT:100>] [<n processes(MAX 20)>] < testing.data\n", argv[0]);
                printf("This program is a license manager, part 2. We'll be doing using this to learn about semaphores\n");
                exit(EXIT_SUCCESS);
            case 't':
                if (!isdigit(argv[2][0])) {
                    //This case the user uses -t parameter but entered a string instead of an int.
                    printf("value entered: %s\n", argv[2]);
                    printf("%s: ERROR: -n <number of processes>\n", argv[0]);
                    exit(EXIT_FAILURE);
                } else {
                    // -t gives us the number of seconds to our timer.
                    tValue = atoi(optarg);
                    // we will check to make sure nValue is 1 to 20.
                    if (tValue < 1) {
                        printf("%s: processes cannot be less than 1. Resetting to default value: 100\n", argv[0]);
                        tValue = MAX_SECONDS;
                    }
                    printf("tValue: %d\n", tValue);
                    break;
                }
            default: /* '?' */
                printf("%s: ERROR: parameter not recognized.\n", argv[0]);
                fprintf(stderr, "Usage: %s [-h] [-t <seconds. DEFAULT:100>] [<n processes(MAX 20)>] < testing.data\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    } /* END OF GETOPT */

    if (isalpha(argv[optind][0])) {
        //This case considers that a person entered a number of some kind or has a number before the letters.
        fprintf(stderr, "n needs to be an integer for number of processes.\n");
        exit(EXIT_FAILURE);
    } else {
        nValue = atoi(argv[optind]);
        printf("nValue: %d\n", nValue);
    }
}

//******************************************* SIGNAL HANDLER FUNCTIONS *******************************************
static void myTimeOutHandler( int s ) {
    char timeout[] = "timing out processes.\n";
    int timeoutSize = sizeof( timeout );
    int errsave;

    errsave = errno;
    write(STDERR_FILENO, timeout, timeoutSize );
    errno = errsave;
    int i;

    //waiting for max amount of children to terminate
    for (i = 0; i <= 20 ; ++i) {
        wait(NULL);
    }

    //detaching shared memory
    if (detachandremove(shmID, sharedHeap) == -1) {
        perror("failed to destroy shared memory segment\n");
        exit(0);
    } else {
        printf("destroyed shared memory segment\n");
    }

    if(removeSem(semID) == -1) {
        perror("failed to remove semaphore\n");
        exit(0);
    } else {
        printf("removed semaphore\n");
    }
    exit(0);
}

static void myKillSignalHandler( int s ){
    char timeout[] = "caught ctrl+c, ending processes.\n";
    int timeoutSize = sizeof( timeout );
    int errsave;

    errsave = errno;
    write(STDERR_FILENO, timeout, timeoutSize );
    errno = errsave;
    int i;

    //waiting for max amount of children to terminate
    for (i = 0; i <= 20; ++i) {
        wait(NULL);
    }
    //detaching shared memory
    if (detachandremove(shmID, sharedHeap) == -1) {
        perror("failed to destroy shared memory segment\n");
        exit(0);
    } else {
        printf("destroyed shared memory segment\n");
    }

    if(removeSem(semID) == -1) {
        perror("failed to remove semaphore\n");
        exit(0);
    } else {
        printf("removed semaphore\n");
    }
    exit(0);
}

static int setupUserInterrupt( void ){
    struct sigaction act;
    act.sa_handler = myKillSignalHandler;
    act.sa_flags = 0;
    return (sigemptyset(&act.sa_mask) || sigaction(SIGINT, &act, NULL));
}

static int setupinterrupt( void ){
    struct sigaction act;
    act.sa_handler = myTimeOutHandler;
    act.sa_flags = 0;
    return (sigemptyset(&act.sa_mask) || sigaction(SIGALRM, &act, NULL));
}

static int setupitimer(int tValue){
    struct itimerval value;
    value.it_interval.tv_sec = tValue;
    value.it_interval.tv_usec = 0;
    value.it_value = value.it_interval;
    return (setitimer( ITIMER_REAL, &value, NULL));
}