//testsim.c

#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include "license.h"

void parsingArgs(int argc, char** argv);                        //Function for parsing arguments

//  SIGNAL HANDLER
static void myKillSignalHandler( int s );                 //This is our signal handler for interrupts
static int setupUserInterrupt( void );

int opt, sleepValue, repeatFactor;

int main(int argc, char** argv){
    printf("exec worked inside ./testsim\n");
    char *log;
    // SETTING UP USER INTERRUPT
    if( setupUserInterrupt() == -1 ){
        perror( "failed to set up a user kill signal.\n");
        return 1;
    }

    int c;
    long myPid = getpid();
    for(c = 0; c < argc; c++){
        printf("arg[%d]: %s\n", c, argv[c]);
    }

    sleepValue = atoi(argv[1]);
    repeatFactor = atoi(argv[2]);
    log = malloc(40*sizeof(char));
    for(c = 1; c != repeatFactor; c++){
        printf("pid: %ld. Sleeping for %d\n", myPid, sleepValue);
        sprintf( log,"%ld  Iteration:%d of %d", myPid, c, repeatFactor);
        printf("%s\n", log);
        //*************************** CRITICAL SECTION *****************************************
        logmsg(log);
        //*************************** CRITICAL SECTION *****************************************
        sleep(sleepValue);
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

    exit(0);
}

static int setupUserInterrupt( void ){
    struct sigaction act;
    act.sa_handler = myKillSignalHandler;
    act.sa_flags = 0;
    return (sigemptyset(&act.sa_mask) || sigaction(SIGINT, &act, NULL));
}