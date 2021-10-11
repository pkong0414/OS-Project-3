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
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <signal.h>
#include "detachandremove.h"
#include "license.h"
#define PERM (IPC_CREAT | S_IRUSR | S_IWUSR)

#define MAX_CANON 13

// FUNCTION PROTOTYPES
void initShm(key_t myKey);                          //This function will initialize shared memory.
void docommand( const int i );                      //This function will manage the giving and receiving of licenses.
void critical_section();                            //This helper function will operate the critical section.
void createChildren();                           //This function will create the children processes from main process
void createGranChildren();                          //This function will be doing the exec functions.
void parsingArgs(int argc, char** argv);            //This helper function will parse command line args.
int max(int numArr[], int n);

// SIGNAL HANDLERS
static void myTimeOutHandler( int s );                    //This is our signal handler for timeouts
static void myKillSignalHandler( int s );                 //This is our signal handler for interrupts
static int setupUserInterrupt( void );
static int setupinterrupt( void );
static int setupitimer( int tValue );

/* THINGS TO DO (Project3):
 *
 * Still make the program interrupts, need Ctrl+c and Timer still!
 * We also need to program a program timer. Default value is 100. Once the time is up,
 * the whole program shuts off no matter what.
 *
 *  We'll need to make semaphores happen now.
 *
 *  1. Create a Makefile that compiles the two source files. [Day 1]
 *  2. Have your main executable read in the command line arguments, validate the arguments, and set up shared memory.
        Also set up the function to deallocate shared memory. Use the command ipcs to make sure that the shared memory
        is allocated and deallocated correctly. [Day 2]
    3. Implement functions to allocate semaphores, use them, and release them. [Day 3]
    4. Get runsim to fork and exec one child and have that child attach to shared memory and read the memory. Reuse the
        testsim code from last project. [Day 4]
    5. Put in the signal handling to terminate after specified number of seconds. A good idea to test this is to simply have
        the child go into an infinite loop so runsim will not ever terminate normally. Once this is working have it catch
        Ctrl-c and free up the shared memory, send a kill signal to the child and then terminate itself. [Day 5]
    6. Set up the code to fork multiple child processes until the specific limits in the loop. Make sure everything works
        correctly. [Day 6]
    7. Make each child process execl testsim. [Day 7]
    8. Create license manager functions. [Day 8]
    9. Integrate the code using semaphores. [Day 9-10]
    10. Test the integrated solution. [Day 11-12]
 */

// GLOBALS
enum state{idle, want_in, in_cs};
int opt, timer, nValue, tValue;                     //This is for managing our getopts
int currentConcurrentProcesses = 1;                 //Initialized as 1 since the main program is also a process.
int totalProcessesCreated = 0;                      //number of created process
int childPid, id, waitStatus;                       //This is for managing our processes
key_t myKey;                                        //Shared memory key
sharedMem *sharedHeap;                              //shared memory object


int main( int argc, char* argv[]){
    //gonna make a signal interrupt here just to see what happens
    if( setupUserInterrupt() == -1 ){
        perror( "failed to set up a user kill signal.\n");
        return 1;
    }

    char command[2][20];
    char buffer[MAX_CANON];
    int i, spaceCount;
//    do{
//        //We'll be using fgets() for our stdin. "testing.data" is what we will be receiving from.
//        fgets(buffer, MAX_CANON, stdin);
//
//        spaceCount = 0;
//        //printing debugging output for fgets received.
//        printf("buffer: %s\n", buffer);
//
//        //clearing the buffer
//        buffer[0] = '\0';
//    }while(!feof(stdin));

    // Getting a new line after reading file
    printf("\n");

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
    initShm(myKey);
    initlicense(sharedHeap);
    addtolicenses(sharedHeap, nValue);

    //creating child processes
    createChildren();

    /* the parent process */

    // waiting for the child process
    while( currentConcurrentProcesses > 1 ) {
        if (wait(&waitStatus) == -1) {
            perror("Failed to wait for child\n");
        } else {
            if (WIFEXITED(waitStatus)) {
                currentConcurrentProcesses--;
                returnlicense(sharedHeap);
                printf("current concurrent process %d\n", currentConcurrentProcesses);
                printf("Child process successfully exited with status: %d\n", waitStatus);
            }
        }
        printf("total processes created: %d\n", totalProcessesCreated);
    }

    //detaching shared memory
    if(detachandremove(id, sharedHeap) == -1){
        perror("Failed to destroy shared memory segment");
        exit(EXIT_FAILURE);
    } else {
        printf("Memory segment detached!\n");
    }

    return 0;
}

void initShm(key_t myKey){
    //********************* SHARED MEMORY PORTION ************************

    if((myKey = ftok(".",1)) == (key_t)-1){
        //if we fail to get our key.
        fprintf(stderr, "Failed to derive key from filename:\n");
        exit(EXIT_FAILURE);
    }
    printf("derived key from, myKey: %d\n", myKey);

    if( (id = shmget(myKey, sizeof(sharedMem), PERM)) == -1){
        perror("Failed to create shared memory segment\n");
        exit(EXIT_FAILURE);
    } else {
        // created shared memory segment!
        printf("created shared memory!\n");

        if ((sharedHeap = (sharedMem *) shmat(id, NULL, 0)) == (void *) -1) {
            perror("Failed to attach shared memory segment\n");
            if (shmctl(id, IPC_RMID, NULL) == -1) {
                perror("Failed to remove memory segment\n");
            }
            exit(EXIT_FAILURE);
        }
        // attached shared memory
        printf("attached shared memory\n");
    }
    //****************** END SHARED MEMORY PORTION ***********************
}

void docommand(const int i){
    //This function will use getlicense() to gather an idea of how many nlicenses are around.
    //If there are 0 then we be blocking until it is ok to receive a license.

    //We'll be following Bakery's Algo for this one
    int j;

    /* Usage: ./testsim [-s seconds for sleep] [-r number of repeats]
     * We'll need to set up just like this
    */
    char *testCallStr;
    testCallStr = "./testsim,testsim,5,10";
    if( getlicense(sharedHeap) == 1 ) {
        do {
            sharedHeap->choosing[i] = 1;
            sharedHeap->number[i] = 1 + sharedHeap->number[max(sharedHeap->number, MAX_PROC)];
            sharedHeap->choosing[i] = 0;
            for (j = 0; j < MAX_PROC; j++) {
                printf("process %d is waiting...\n", i);
                while (sharedHeap->choosing[j]);                //wait while someone is choosing
                while ((sharedHeap->number[j]) &&
                       (sharedHeap->number[j], j) < (sharedHeap->number[i], i));
                sleep(1);
            }

            //critical section time!
            critical_section();

            printf("process %d received the license!\n", i);
            sharedHeap->number[i] = 0;                          //giving up the number
            execl(testCallStr, NULL);

            perror("exec failed");
            //remainder section
            return;
        } while (1);
        return;
    } else {
        printf("process %d received the license!\n", i);
        execl("./testsim", "testsim", "5", "10", NULL);

        perror("exec failed");
        return;
    }
}

void critical_section(){
    if(getlicense(sharedHeap) == 0){
        printf("We have 0 license.\n");
    } else {
        printf("We have our license!\n");
    }
}

void createChildren(){
    int myID = 0;

    if(currentConcurrentProcesses <= MAX_PROC) {
        //changed this from while to if, This way it will only create 1 child!
        if ((childPid = fork()) == -1) {
            perror("Failed to create child process\n");
            if (detachandremove(id, sharedHeap) == -1) {
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
            printf("current concurrent process %d: myPID: %ld\n", currentConcurrentProcesses, (long)getpid());

            //calling the docommand to handle the licensing.
            docommand(myID);

            //exiting child process
            exit(EXIT_SUCCESS);
        }
    }
}

void createGrandChildren(){

}

int max(int numArr[], int n)
{
    static int max=0;
    int i = 0;
    for(i; i < n; i++) {
        if(numArr[max] < numArr[i]) {
            max=i;
        }
    }

    return max;

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
                        tValue = 100;
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

static void myTimeOutHandler( int s ) {
    char timeout[] = "timing out processes.\n";
    int timeoutSize = sizeof( timeout );
    int errsave;

    errsave = errno;
    write(STDERR_FILENO, timeout, timeoutSize );
    errno = errsave;
    int i;

    //waiting for max amount of children to terminate
    for (i = 0; i <= nValue ; ++i) {
        wait(NULL);
    }
    //detaching shared memory
    if (detachandremove(id, sharedHeap) == -1) {
        perror("failed to destroy shared memory segment\n");
        exit(0);
    } else {
        printf("destroyed shared memory segment\n");
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
    for (i = 0; i <= nValue ; ++i) {
        wait(NULL);
    }
    //detaching shared memory
    if (detachandremove(id, sharedHeap) == -1) {
        perror("failed to destroy shared memory segment\n");
        exit(0);
    } else {
        printf("destroyed shared memory segment\n");
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