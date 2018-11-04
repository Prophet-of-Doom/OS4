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
int alrm;
void timerKiller(int sign_no){
        alrm = 1;
}
	
void setRandomForkTime(unsigned int *seconds, unsigned int *nanoseconds, int *forkTimeSeconds, int *forkTimeNanoseconds){
        int randnum = rand()%3; // 0 to 2
	*forkTimeSeconds = 0;
	*forkTimeNanoseconds = 0;
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
int checkForRunning(PCB *pcbPtr,int *processRunning){
	int i = 0;
	for (i = 0; i < 18; i++){
		if(pcbPtr[i].isRunning == 1){
			*processRunning = 1;
			printf("PROCESS %d PID: %d IS RUNNING\n", i, pcbPtr[i].pid); 
			return *processRunning;
		}
	}
	*processRunning = 0;
	return *processRunning;
}
int isVectorFull(int bitVector[],int *isFull){
        int i = 0;
        for (i = 0; i < 8; i++){
                if(bitVector[i] == 0){
                        *isFull = 0;
                        return *isFull;
                } 
		*isFull = 1;
        }
        return *isFull;
}
int main(int argc, char *argv[]){
	srand(time(NULL));
	alrm = 0;
	
	char* filename = malloc(sizeof(char));
	filename = "log.txt";
        FILE *logFile = fopen(filename, "a");
	
	int bitVector[18] = {0};
	key_t strctkey = 0, semkey = 0, timekey = 0, qkey = 0;
	int strctid = 0, randomSet = 0, isFull = 1, processRunning = 0, semid = 0, status = 0, timeid = 0, position = 0, forkTimeSeconds = 0, forkTimeNanoseconds = 0, qid = 0, terminatingPID = 0, processTerminating = 0; 
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
      	initLPQueue();
	initHPQueue();
 	*quantum = 1000000;
	int forked = 0;
	//signal(SIGALRM, timerKiller);
        //alarm(2);
	do{
		printf("!!!!Seconds: %d Nano: %d\n", *seconds, *nanoseconds);
		printf("BEFORE WAIT\n");
		sem_wait(sem);
		printf("AFTER WAIT\n");
		isVectorFull(bitVector, &isFull);		
		//printf("!!!!Seconds: %d Nano: %d\n", *seconds, *nanoseconds);
		//sleep(1);
		if(isFull == 0){
			if(randomSet == 0){
				setRandomForkTime(seconds, nanoseconds, &forkTimeSeconds, &forkTimeNanoseconds);
				randomSet = 1;
			}
			checkForRunning(pcbPtr, &processRunning);
			//printf("process running");
			if(processRunning == 0){
				incrementClock(&seconds, &nanoseconds);
				if(*nanoseconds >= 1000000000){//billion
					(*seconds)++;
					*nanoseconds = 0;
				}
			}
			if((*seconds >= forkTimeSeconds)||(*seconds >= forkTimeSeconds && *nanoseconds >= forkTimeNanoseconds)) {
				printf("created a child at %d seconds and %d nanoseconds\n", *seconds, *nanoseconds);
				//sleep(1);
				checkArrPosition(pcbPtr, &position);
				//printf("position %d\n", position);
				createArgs(sharedStrctMem, sharedSemMem, sharedTimeMem, qMem, sharedArrayPosition, strctid, semid, timeid, qid, position);
				forkChild(&pcbPtr ,pid, sharedStrctMem, sharedSemMem, sharedTimeMem, qMem, sharedArrayPosition, &position, seconds, nanoseconds, logFile);	
				forked++;
				randomSet = 0;
			}
			//sleep(1);
			enqueueReadyProcess(pcbPtr);
			if(isHPQueueEmpty() == 0){
				fprintf(logFile, "OSS: Dequeueing user %u at %u.%u\n", pcbPtr[position].pid, *seconds
, *nanoseconds);
				dequeueHP(pcbPtr, &position);
				pcbPtr[position].flag = 1;
			} else if (isLPQueueEmpty() == 0){
				fprintf(logFile, "OSS: Dequeueing user %u at %u.%u\n", pcbPtr[position].pid, *seconds
, *nanoseconds);
				dequeueLP(pcbPtr, &position);
				pcbPtr[position].flag = 1;
			}
			checkForTerminatingProcesses(pcbPtr, &position, &terminatingPID, &processTerminating);
			if(processTerminating == 1){
				fprintf(logFile, "OSS: Killing process %u at %u.%u\n", pcbPtr[position].pid, *seconds
, *nanoseconds);
				addUserCPUTimeToClock(pcbPtr, position, seconds, nanoseconds);
				printf("X BEFORE WAITPID FOR %d\n", pcbPtr[position].pid);
				//sem_wait(sem);
				pcbPtr[position].msgReceived = 1;
				//sem_post(sem);
				printf("X after sem_wait for terminating");
				fprintf(logFile, "OSS: process %u was alive for %u.%u\n", pcbPtr[position].pid, *seconds
, *nanoseconds);
				if(pcbPtr[position].terminating == 1){
					if(waitpid(pcbPtr[position].pid, &status, WUNTRACED) == terminatingPID){
						//CLEAR PCB SHIT
						printf("AFTER WAITPID\n");
						clearPCB(pcbPtr, position);
						int i;
						for(i = 0; i < 18; i++){
							if(bitVector[i] == 1){
								bitVector[i] = 0;
							}
						}
					} else if(waitpid(pcbPtr[position].pid, &status, WUNTRACED) == -1){
       	                               		//CLEAR PCB SHIT	
       	                        	        clearPCB(pcbPtr, position);
						int i;
       	                	                for(i = 0; i < 18; i++){
       	        	                        	if(bitVector[i] == 1){
       	                                        		bitVector[i] = 0;
       	                                    	    	}	
       		                        	}
						kill(pcbPtr[position].pid, SIGTERM);
					}
					kill(pcbPtr[position].pid, SIGTERM);
				}
			}
		} else if(isFull == 1){
			printf("isFull is 1\n");
			checkForRunning(pcbPtr, &processRunning);
			if(processRunning == 0){
                                incrementClock(&seconds, &nanoseconds);
                                if(*nanoseconds >= 1000000000){//billion
                                        (*seconds)++;
                                        *nanoseconds = 0;
                                }
                        }
			enqueueReadyProcess(pcbPtr);
                        if(isHPQueueEmpty() == 0){
				fprintf(logFile, "OSS: Dequeueing user %u at %u.%u\n", pcbPtr[position].pid, *seconds, *nanoseconds);
                                dequeueHP(pcbPtr, &position);
				printf("PCB FLAG POS %d\n", pcbPtr[position].flag);
                                pcbPtr[position].flag = 1;
                        } else if (isLPQueueEmpty() == 0){
				fprintf(logFile, "OSS: Dequeueing user %u at %u.%u\n", pcbPtr[position].pid, *seconds, *nanoseconds);
                                dequeueLP(pcbPtr, &position);	
				printf("PCB FLAG POS %d\n", pcbPtr[position].flag);
                                pcbPtr[position].flag = 1;
                        }
                        checkForTerminatingProcesses(pcbPtr, &position, &terminatingPID, &processTerminating);
                        if(processTerminating == 1){
                                addUserCPUTimeToClock(pcbPtr, position, seconds, nanoseconds);
				pcbPtr[position].msgReceived = 1;
				fprintf(logFile, "OSS: killing process %d\n", pcbPtr[position].pid);
                                if(waitpid(pcbPtr[position].pid, &status, WUNTRACED) == terminatingPID){
                                        int i;
        	                        clearPCB(pcbPtr, position);        
					for(i = 0; i < 18; i++){
                                                if(bitVector[i] == 1){
                                                        bitVector[i] = 0;
                                                }
                                        }
                                } else if(waitpid(pcbPtr[position].pid, &status, WUNTRACED) == -1){
                                        int i;
					clearPCB(pcbPtr, position);
                                        for(i = 0; i < 18; i++){
                                                if(bitVector[i] == 1){
                                                        bitVector[i] = 0;
                                                }
                                        }
                                        kill(pcbPtr[position].pid, SIGTERM);
                                }
                        }
		}
		printf("FORKED: %d\n", forked);
		sem_post(sem);
	}while((forked < 100) && alrm == 0); 
	int i = 0;
	for(i = 0; i < 2; i++){
		//printf("The total struct values are: \n totalCpuTime %d\n totalTimeAlive %d\n timeSinceLastBurst %d\n processPriority %d\n position %d\n flag %d\n isSet %d\n terminating %d\n pid %d\n", pcbPtr[i].totalCpuTime, pcbPtr[i].totalTimeAlive, pcbPtr[i].timeSinceLastBurst, pcbPtr[i].processPriority, pcbPtr[i].position, pcbPtr[i].flag, pcbPtr[i].isSet, pcbPtr[i].terminating, pcbPtr[i].pid);
	}
	printf("OSS: OUT OF LOOP\n");	
	//while((pid = wait(&status) > 0));
	//int i = 0;
	//for(i = 0; i < 18; i++) 
	//	printf("The value is %d\n", pcbPtr[i].position);
 	fclose(logFile);
        sem_destroy(sem);
        shmdt(pcbPtr);
        shmdt(sem);
	shmdt(seconds);
	shmdt(quantum);
	shmctl(qid, IPC_RMID, NULL);
        shmctl(strctid, IPC_RMID, NULL);
        shmctl(semid, IPC_RMID, NULL);
	shmctl(timeid, IPC_RMID, NULL);
	kill(0, SIGTERM);
	return 0;
}
