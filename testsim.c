//testsim.c

#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "license.h"

void parsingArgs(int argc, char** argv);                        //Function for parsing arguments

int opt, sleepValue, repeatFactor;

int main(int argc, char** argv){
    printf("exec worked inside ./testsim\n");
    int c;
    long myPid = getpid();
    for(c = 0; c < argc; c++){
        printf("arg[%d]: %s\n", c, argv[c]);
    }

    sleepValue = atoi(argv[1]);
    repeatFactor = atoi(argv[2]);

    for(c = 0; c < repeatFactor; c++){
        printf("pid: %ld. Sleeping for %d\n", myPid, sleepValue);
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