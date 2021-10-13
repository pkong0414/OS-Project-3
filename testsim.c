//testsim.c

#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include "license.h"
#define PERM (IPC_CREAT | S_IRUSR | S_IWUSR)

void parsingArgs(int argc, char** argv);                        //Function for parsing arguments

//  SIGNAL HANDLER
static void myKillSignalHandler( int s );                       //This is our signal handler for interrupts
static int setupUserInterrupt( void );
void initSem();                                         //This is going to initialize our semaphores.
void semWait();
void semSignal();
static void setsembuf(struct sembuf *s, int num, int op, int flg);
int r_semop(int semid, struct sembuf *sops, int nsops);

//  SEMAPHORE
key_t myKey;
int semID;
struct sembuf writeP;
struct sembuf writeV;

int opt, sleepValue, repeatFactor;

int main(int argc, char** argv){
    int error;
    printf("exec worked inside ./testsim\n");
    char *log;
    // SETTING UP USER INTERRUPT
    if( setupUserInterrupt() == -1 ){
        perror( "failed to set up a user kill signal.\n");
        return 1;
    }

    initSem();
    int c;
    long myPid = getpid();

    for(c = 0; c < argc; c++){
        printf("arg[%d]: %s\n", c, argv[c]);
    }

    sleepValue = atoi(argv[1]);
    repeatFactor = atoi(argv[2]);
    log = malloc(40*sizeof(char));
    for(c = 1; c <= repeatFactor; c++){
        printf("pid: %ld. Sleeping for %d\n", myPid, sleepValue);
        sprintf( log,"%ld  Iteration:%d of %d", myPid, c, repeatFactor);
        printf("%s\n", log);
        sleep(sleepValue);
        //*********************************** ENTRY SECTION ********************************************
        semWait();
        //********************************** CRITICAL SECTION ******************************************
        printf("pid: %ld. in critical section\n", myPid);
        sleep(1);
        logmsg(log);
        //********************************** EXIT SECTION **********************************************
        printf("pid: %ld. exited critical section\n", myPid);
        semSignal();
        //******************************** REMAINDER SECTION *********************************************

    }

    return 0;
}

void parsingArgs(int argc, char** argv){
    while((opt = getopt(argc, argv, "s:r:")) != -1) {
        switch (opt) {
            case 's':
                if (!isdigit(argv[2][0])) {
                    //This case the user uses -t parameter but entered a string instead of an int.
                    printf("value entered: %s\n", argv[2]);
                    printf("%s: ERROR: -s <seconds for sleep>\n", argv[0]);
                    exit(EXIT_FAILURE);
                } else {
                    // -n gives us the number of licenses available
                    sleepValue = atoi(optarg);
                    // we will check to make sure nValue is 1 to 20.
                    printf("sleepValue: %d\n", sleepValue);
                    break;
                }
            case 'r':
                if (!isdigit(argv[3][0])) {
                    //This case the user uses -t parameter but entered a string instead of an int.
                    printf("value entered: %s\n", argv[2]);
                    printf("%s: ERROR: -r <number of repeats>\n", argv[0]);
                    exit(EXIT_FAILURE);
                } else {
                    // -n gives us the number of licenses available
                    repeatFactor = atoi(optarg);
                    // we will check to make sure nValue is 1 to 20.
                    printf("repeatFactor: %d\n", repeatFactor);
                    break;
                }
            default: /* '?' */
                printf("%s: ERROR: parameter not recognized.\n", argv[0]);
                fprintf(stderr, "Usage: %s [-s seconds for sleep] [-r number of repeats]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    } /* END OF GETOPT */
}

static void myKillSignalHandler( int s ){
    char timeout[] = "caught ctrl+c, ending processes.\n";
    int timeoutSize = sizeof( timeout );
    int errsave;

    errsave = errno;
    write(STDERR_FILENO, timeout, timeoutSize );
    errno = errsave;

    exit(1);
}

static int setupUserInterrupt( void ){
    struct sigaction act;
    act.sa_handler = myKillSignalHandler;
    act.sa_flags = 0;
    return (sigemptyset(&act.sa_mask) || sigaction(SIGINT, &act, NULL));
}

void initSem(){
    if((myKey = ftok(".",1)) == (key_t)-1){
        //if we fail to get our key.
        fprintf(stderr, "Failed to derive key from filename:\n");
        exit(EXIT_FAILURE);
    }
    printf("derived key from, myKey: %d\n", myKey);

    if( (semID = semget(myKey, 1, PERM | IPC_CREAT)) == -1){
        perror("Failed to create semaphore with key\n");
        exit(EXIT_FAILURE);
    } else {
        printf("Semaphore created with key\n");
    }
    //setsembuf(&writeP, 0, -1, 0);
    //setsembuf(&writeV, 0, 1, 0);
}

void semWait(){
    if (semop(semID, &writeP, 1) == -1) {
        perror("./testSim semop wait");
        exit(1);
    }
}

void semSignal(){
    if (semop(semID, &writeV, 1) == -1) {
        perror("./testSim semop signal");
        exit(1);
    }
}


void setsembuf(struct sembuf *s, int num, int op, int flg){
    s->sem_num = (short)num;
    s->sem_op = (short)op;
    s->sem_flg = (short)flg;
    return;
}

int r_semop(int semid, struct sembuf *sops, int nsops) {
    while (semop(semid, sops, nsops) == -1) {
        if (errno != EINTR) {
            return -1;
        }
    }
    return 0;
}