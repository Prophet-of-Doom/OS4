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
	int PCBposition = atoi(argv[5]);
	int *quantum = 0;
	unsigned int *seconds = 0, *nanoseconds = 0;
        PCB *pcbPtr = NULL;
        sem_t *sem = NULL;
	int localQuantum = 0;
	unsigned int tempTimeInSystem_seconds = 0, tempTimeInSystem_nano = 0, tempTimeSinceLastBurst_seconds = 0, tempTimeSinceLastBurst_nano = 0, waitToQueue_seconds = 0, waitToQueue_nano = 0; 		
        attachToSharedMemory(&pcbPtr, &sem, &seconds, &nanoseconds, &quantum, semid, strctid, timeid, qid);
	setQuantum(pcbPtr, PCBposition, quantum, &localQuantum);
        initializeSemaphores(&sem);
	
	pcbPtr[PCBposition].position = PCBposition;	
	sem_wait(sem);
	initializeUser(pcbPtr, PCBposition, &seconds, &nanoseconds, &tempTimeInSystem_seconds, &tempTimeInSystem_nano);	
	sem_post(sem);
	printf("wow\n");
		
	while(pcbPtr[PCBposition].msgReceived == 0){
		sem_wait(sem); 
		if(pcbPtr[PCBposition].flag == 1){
			printf("In here wtf\n");
			//sem_wait(sem)
			pcbPtr[PCBposition].isRunning = 1;
			//getAction();
			randomAction(pcbPtr, localQuantum, PCBposition);
			howMuchQuantum(pcbPtr, localQuantum, PCBposition);
			//sem_post(sem);
			pcbPtr[PCBposition].isRunning = 0;
			sem_post(sem);
			break;
		}
		//pcbPtr[PCBposition].flag = 1;
		
		sem_post(sem);
	}
		
	printf("User Position: %d PID: %d\n",PCBposition, pcbPtr[PCBposition].pid);
        pcbPtr[PCBposition].terminating = 0;
	sem_destroy(sem);
        shmdt(pcbPtr);
        shmdt(sem);
 
	return 0;
}
