// license.h

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include "config.h"

#ifndef LICENSE_H
#define LICENSE_H

// shared memory struct
typedef struct sharedMemory {
    bool choosing[MAX_PROC];                        //This will be the queue for the processes.
    int number[MAX_PROC];                           //This is the current turn of the waiting line.
    int nlicense;                                   //This is license object. This is the critical section object
} sharedMem;

// functions
int getlicense(sharedMem *);                        //Blocks until a license is available
int returnlicense(sharedMem *);                     //Increments the number of avail license object
int initlicense(sharedMem *);                       //Performs any needed initialization of the license object
void addtolicenses(sharedMem *, int n);             //Adds n licenses to the number available
void removelicenses(sharedMem *, int n);            //Decrements the number of licenses by n
void logmsg(const char* msg);                       //Write the specified message to the log file.
char *getTime();                                    //Helper function to logmsg. It just returns time in a format

/* NOTE: There is only one log file. This function will treat the log file as a critical resource.
 * It will open the file to append the message and close the file after appending the message.
 *
 */


#endif