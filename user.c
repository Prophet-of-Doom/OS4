#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
 
#include "header.h"
 
int main(int argc, char *argv[]) {

        int strctid = atoi(argv[1]);
        int semid = atoi(argv[2]);
	int timeid = atoi(argv[3]);
	int qid = atoi(argv[4]);
	int position = atoi(argv[5]);
	int *quantum = 0;
	unsigned int *seconds = 0, *nanoseconds = 0;
        PCB *pcbPtr = NULL;
        sem_t *sem = NULL;
 	
        attachToSharedMemory(&pcbPtr, &sem, &seconds, &nanoseconds, &quantum, semid, strctid, timeid, qid);
        initializeSemaphores(&sem);

	sem_wait(sem);
 	pcbPtr[position].pid = getpid();
	pcbPtr[position].isSet = 1;
	pcbPtr[position].totalCpuTime = 100;
	//pcbPtr[position].totalTimeAlive = 110;
	//pcbPtr[position].timeSinceLastBurst = 120;
	//pcbPtr[position].processPriority = 130;	
	sem_post(sem);
	printf("%d %d\n", pcbPtr[position].position, position);
        sem_destroy(sem);
        shmdt(pcbPtr);
        shmdt(sem);
 
	return 0;
}
