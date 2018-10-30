#include "header.h"
#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

void setRandomForkTime(unsigned int *seconds, unsigned int *nanoseconds, int *forkTimeSeconds, int *forkTimeNanoseconds){
        int randnum = rand()%3; // 0 to 2
        *forkTimeSeconds = (*seconds) + randnum;
	*forkTimeNanoseconds = *nanoseconds;
}

void checkArrPosition(PCB *pcbPtr, int *position){
        int i = 0;
        for(i = 0; i < 18; i++){
                if(pcbPtr[i].isSet == 0){
                        *position = i;
                        break;
                }
        }
}

void incrementClock(unsigned int **seconds, unsigned int **nanoseconds){
        (**seconds)++;
        **nanoseconds += rand()%1001;
}

int main(int argc, char *argv[]){
	srand(time(NULL));
	int bitVector[18] = {0};
	key_t strctkey = 0, semkey = 0, timekey = 0, qkey = 0;
	int strctid = 0, isFull = 0, semid = 0, status = 0, timeid = 0, position = 0, forkTimeSeconds = 0, forkTimeNanoseconds = 0, qid = 0; 
	int *quantum = 0;
	unsigned int *seconds = 0, *nanoseconds = 0;
	PCB *pcbPtr = NULL;
	sem_t *sem = NULL;
	pid_t pid = 0;

	char sharedStrctMem[10], sharedSemMem[10], sharedTimeMem[10], sharedArrayPosition[10], qMem[10];

	 
        createSharedMemKeys(&strctkey, &semkey, &timekey, &qKey);
        createSharedMemory(&strctid, &semid, &timeid, &qid, strctkey, semkey, timekey, qkey);
        attachToSharedMemory(&pcbPtr, &sem, &seconds, &nanoseconds, &quantum, semid, strctid, timeid, qid);
        
        initializeSemaphores(&sem);
       
	int forked = 0;
	do{
		sem_wait(sem);
		if(isFull == 0){
			setRandomForkTime(seconds, nanoseconds, &forkTimeSeconds, &forkTimeNanoseconds);
			incrementClock(&seconds, &nanoseconds);
			if(*nanoseconds >= 1000000000){//billion
				(*seconds)++;
				*nanoseconds = 0;
			}
			if((*seconds >= forkTimeSeconds)||(*seconds >= forkTimeSeconds && *nanoseconds >= forkTimeNanoseconds)) {
			printf("created a child at %d seconds and %d nanoseconds\n", *seconds, *nanoseconds);
			checkArrPosition(pcbPtr, &position);
			printf("position %d\n", position);
			createArgs(sharedStrctMem, sharedSemMem, sharedTimeMem, qMem, sharedArrayPosition, strctid, semid, timeid, qid, position);
			forkChild(pid, sharedStrctMem, sharedSemMem, sharedTimeMem, qMem,sharedArrayPosition, &position);	
			forked++;
			}
		} else {
			printf("OSS is exiting loop\n");
			break;
		}
		sem_post(sem);
		sleep(1);
	}while((forked < 2) && (*seconds < 1000000000)); 
	int i = 0;
	for(i = 0; i < 2; i++){
		printf("The total struct values are: \n totalCpuTime %d\n totalTimeAlive %d\n timeSinceLastBurst %d\n processPriority %d\n position %d\n flag %d\n isSet %d\n terminating %d\n pid %d\n", pcbPtr[i].totalCpuTime, pcbPtr[i].totalTimeAlive, pcbPtr[i].timeSinceLastBurst, pcbPtr[i].processPriority, pcbPtr[i].position, pcbPtr[i].flag, pcbPtr[i].isSet, pcbPtr[i].terminating, pcbPtr[i].pid);
	}	
	while((pid = wait(&status) > 0));
	//int i = 0;
	//for(i = 0; i < 18; i++) 
	//	printf("The value is %d\n", pcbPtr[i].position);
 
        sem_destroy(sem);
        shmdt(pcbPtr);
        shmdt(sem);
	shmdt(seconds);
	shmdt(quantum);
	shmctl(qid, IPC_RMID, NULL);
        shmctl(strctid, IPC_RMID, NULL);
        shmctl(semid, IPC_RMID, NULL);
	shmctl(timeid, IPC_RMID, NULL);
	return 0;
}
