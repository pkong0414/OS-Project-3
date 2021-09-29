//detachandremove.c

#include <stdio.h>
#include <errno.h>
#include <sys/shm.h>
#include "detachandremove.h"

int detachandremove(int shmid, void *shmaddr){
    int error = 0;

    if(shmdt(shmaddr) == -1){
        printf("Failed to detach shared memory address\n");
        error = errno;
    }
    if((shmctl(shmid, IPC_RMID, NULL) == -1) && !error){
        printf("Failed to detach shared memory id\n");
        error = errno;
    }
    if(!error){
        return 0;
    }
    errno = error;
    return -1;
}