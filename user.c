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
	pid_t pid = getpid();
	unsigned int *seconds = 0, *nanoseconds = 0;
        PCB *pcbPtr = NULL;
        sem_t *sem = NULL;
	int localQuantum = NULL;
	unsigned int tempTimeInSystem_seconds = 0, tempTimeInSystem_nano = 0, tempTimeSinceLastBurst_seconds = 0, tempTimeSinceLastBurst_nano = 0, waitToQueue_seconds = 0, waitToQueue_nano = 0; 		
        attachToSharedMemory(&pcbPtr, &sem, &seconds, &nanoseconds, &quantum, semid, strctid, timeid, qid);
	initializeSemaphores(&sem);
	
	sem_wait(sem);
	pcbPtr[PCBposition].position = PCBposition;
	initializeUser(pcbPtr, PCBposition, &seconds, &nanoseconds, &tempTimeInSystem_seconds, &tempTimeInSystem_nano);	
	setQuantum(pcbPtr, PCBposition, quantum, &localQuantum);
	getAction(&pcbPtr, PCBposition, &seconds, &nanoseconds, quantum, &waitToQueue_seconds, &waitToQueue_nano, &tempTimeInSystem_seconds, &tempTimeInSystem_nano, &tempTimeSinceLastBurst_seconds, &tempTimeSinceLastBurst_nano);
	sem_post(sem);
	if(pcbPtr[PCBposition].terminating == 1){
		printf("User Position: %d PID: %d IS TERMINATING\n",PCBposition, pcbPtr[PCBposition].pid);
        	pcbPtr[PCBposition].terminating = 0;
        	sem_destroy(sem);
        	shmdt(pcbPtr);
        	shmdt(sem);	
	}
	printf("wow\n");
	
	while(pcbPtr[PCBposition].waitingToQueue == 1){
		pcbPtr[PCBposition].isRunning = 0;
		printf("PID %d is waiting to queue at sec %d nano %d\n", pid, waitToQueue_seconds, waitToQueue_nano);
		sem_wait(sem);
		if((*seconds == waitToQueue_seconds && *nanoseconds >= waitToQueue_nano) || (*seconds > waitToQueue_seconds)){
			sem_post(sem);
			break;
		}
		sem_post(sem);
	}
	pcbPtr[PCBposition].waitingToQueue = 0;
	//pcbPtr[PCBposition].isRunning = 1;
	howMuchQuantum(pcbPtr, &localQuantum, PCBposition);
	localQuantum += *seconds;
	while(1){
		sem_wait(sem);
		pcbPtr[PCBposition].isRunning = 1;
		if(pcbPtr[PCBposition].flag == 1){
			while(1){//pcbPtr[PCBposition].msgReceived == 0){
				sem_wait(sem); 
				//pcbPtr[PCBposition].isRunning = 1;
				if(localQuantum >= *seconds){
					pcbPtr[PCBposition].terminating = 1;
					pcbPtr[PCBposition].isRunning = 0;
					sem_post(sem);
					break;
				}
				/*if(pcbPtr[PCBposition].flag == 1){
					printf("In here wtf\n");
					//sem_wait(sem)
					pcbPtr[PCBposition].isRunning = 1;
					//getAction();	
					howMuchQuantum(pcbPtr, localQuantum, PCBposition);
					//sem_post(sem);
					pcbPtr[PCBposition].isRunning = 0;
					sem_post(sem);
					break;
				}*/
				//pcbPtr[PCBposition].flag = 1;
				
				//pcbPtr[PCBposition].isRunning = 0;
				sem_post(sem);
			}
		}
		if(pcbPtr[PCBposition].terminating == 1 && pcbPtr[PCBposition].isRunning == 0){
			break;
		} else {
			pcbPtr[PCBposition].isRunning = 0;
			sem_post(sem);
		}
	}
	
	while(1){
		if(pcbPtr[PCBposition].msgReceived == 1){
			getUserTotalSystemTime(sptr, position, seconds, nanoseconds, &tempTimeInSystem_seconds, &tempTimeInSystem_nano);
		       	 getTimeSinceLastBurst( sptr, position, seconds, nanoseconds, &tempTimeSinceLastBurst_seconds, &tempTimeSinceLastBurst_nano);	
			printf("User Position: %d PID: %d IS TERMINATING\n",PCBposition, pcbPtr[PCBposition].pid);
		       	 pcbPtr[PCBposition].terminating = 0;
			sem_destroy(sem);
       		 	shmdt(pcbPtr);
       			 shmdt(sem);
			break;
		}
 	}
	return 0;
}
