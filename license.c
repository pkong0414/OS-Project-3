//license.c

#include "license.h"

int getlicense(sharedMem* sharedHeap){
    //This function will detect the number of licenses available.
    //If there are no available licenses, it will block any processes from taking a license.
    if( sharedHeap->nlicense == 0 ){
        return 1;
    } else {
        //we have licenses now.
        sharedHeap->nlicense--;
        printf("we now have %d number of licenses\n", sharedHeap->nlicense);
        return 0;
    }
}

int returnlicense(sharedMem* sharedHeap){
    printf("Returning license\n");
    sharedHeap->nlicense++;
    printf("current available license: %d\n", sharedHeap->nlicense);
}

int initlicense(sharedMem *sharedHeap){
    printf("initializing license to 0\n");
    sharedHeap->nlicense = 0;
    return 0;
}

void addtolicenses(sharedMem *sharedHeap, int n){
    sharedHeap->nlicense += n;
    printf("added %d licenses, current nlicense: %d\n", n, sharedHeap->nlicense);
}

void removelicenses(sharedMem* sharedHeap,int n){
    sharedHeap->nlicense -= n;
    printf("subtracted %d licenses, current nlicense: %d\n", n, sharedHeap->nlicense);
}

void logmsg(const char* msg){
    /* logmsg in the format:
    *   Time PID Iteration# of NumberOfIterations
    */
    //The savelog function saves the logged message to a disk file.
    char* filename = "temp.dat";
    FILE *saveFile;
    char *formattedTime;
    saveFile = fopen(filename, "w");

    if(saveFile == NULL){
        perror("log.c: ERROR");
        //return -1 if unsuccessful
        return;
    }
    else {
        //Now writing to file
        fprintf( saveFile, "%s\n", msg);
        //closing file
        if(fclose(saveFile) == -1){
            perror("log.c: ERROR");

            //exiting since file is unable to be closed
            exit(EXIT_FAILURE);
        }
        else {
            printf("File saved to: %s\n", filename);
            //return 0 if successful
            return;
        }
    }
}

char *getTime(){
    // getting our total seconds
    time_t epoch_seconds;
    epoch_seconds = time(NULL);
    int hours;
    int minutes;
    int seconds;
    char* timestamp;

    // calculations will be performed such that we will have time in format of:
    // HH:MM:SS
    // To calculate hours we'll do (epoch_time / 3600) % 24
    // To calculate minutes we'll do (epoch_time / 60) % 60
    // To calculate seconds we'll do (epoch_time % 60)

    // Hours needs to be -5 (we are cst local time and we are converting from UTC)
    hours = ((epoch_seconds / 3600) % 24) - 5;
    minutes = ((epoch_seconds / 60) % 60);
    seconds = (epoch_seconds % 60);
    timestamp = malloc(10*sizeof(char) + 1);

    // converting the newly acquired time units into specified format of: HH:MM:SS for timestamp.
    sprintf( timestamp, "[%02u:%02u:%02u]", hours, minutes, seconds);

    return timestamp;
}