#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <time.h>
#include <stdlib.h>
#include <limits.h>
#define STRUCT_ARRAY_SIZE ((sizeof(pcbArray)/sizeof(pcbArray[0])) * 18)
#define SEM_SIZE sizeof(int)
#define QUEUE_SIZE 18
//for the queue
#ifndef FALSE
#define FALSE (0)
#endif
//For true
#ifndef TRUE
#define TRUE (!FALSE)
#endif
typedef struct PCB{
	unsigned int totalCPUTime_seconds;
	unsigned int totalCPUTime_nano;
	unsigned int totalTimeInSystem_seconds;
	unsigned int totalTimeInSystem_nano;
	unsigned int timeSinceLastBurst_nano;
	unsigned int timeSinceLastBurst_seconds;
	unsigned int totalCPUTimeUsed_nano;
	unsigned int totalCPUTimeUsed_seconds;
	int processPriority;
	int position;
	int flag;
	int waitingToQueue;	
	int isSet;
	int terminating;
	int isRunning;
	int msgReceived;
	pid_t pid;
}PCB;

typedef struct QNodeType{
	PCB *qptr;
	struct QNodeType *next;
}QNode;

int enqueueHP(PCB *sptr, int position);

int enqueueLP(PCB *sptr, int position);

pid_t dequeueHP(PCB *sptr, int *position);
void getUserTotalSystemTime(PCB *sptr, int position, unsigned int *seconds, unsigned int *nanoseconds, unsigned int *tempTimeInSystem_seconds, unsigned int *tempTimeInSystem_nano);
pid_t dequeueLP(PCB *sptr, int *position);

PCB pcbArray[18];
PCB *sptr = NULL;
sem_t *sem = NULL;
pid_t pid = 0;
key_t strctKey = 0, semKey = 0, timeKey = 0, posKey = 0, qKey = 0;
unsigned int *seconds = 0, *nanoseconds = 0, *randsecs = 0, *randnanosecs = 0;
int *quantum = 0; 
static QNode *headLP, *tailLP, *headHP, *tailHP;
//static pid_t theLPQueue[QUEUE_SIZE], theHPQueue[QUEUE_SIZE];
int strctid = 0, semid = 0, timeid = 0, qid = 0,position = 0, bitVector[18];
char strctMem[10], semMem[10], timeMem[10], qMem[10], arrayPosition[10];

void getTimeToWait(unsigned int *seconds, unsigned int *nanoseconds, unsigned int *waitToQueue_seconds, unsigned int *waitToQueue_nano){
	unsigned int tempSeconds =  (rand()%6);
	unsigned int tempNano = (rand()%1001);
	printf("temp seconds %d temp nano %d", tempSeconds, tempNano);
	*waitToQueue_seconds = seconds[0]+tempSeconds;
	*waitToQueue_nano = nanoseconds[0]+tempNano;
	if(*waitToQueue_nano >= 1000000000){
		*waitToQueue_seconds += 1;
		*waitToQueue_nano -= 1000000000;
	}
}

void getTimeSinceLastBurst(PCB *sptr, int position, unsigned int *seconds, unsigned int *nanoseconds, unsigned int *tempTimeSinceLastBurst_seconds, unsigned int *tempTimeSinceLastBurst_nano){
	if(sptr[position].timeSinceLastBurst_seconds == 0 && sptr[position].timeSinceLastBurst_nano == 0){
		*tempTimeSinceLastBurst_seconds = seconds[0];
		*tempTimeSinceLastBurst_nano = nanoseconds[0];
	} else {
		sptr[position].timeSinceLastBurst_seconds = seconds[0] - *tempTimeSinceLastBurst_seconds;
		if(nanoseconds[0] < *tempTimeSinceLastBurst_nano){
			sptr[position].timeSinceLastBurst_nano = sptr[position].timeSinceLastBurst_nano - nanoseconds[0];
		} else {
			sptr[position].timeSinceLastBurst_nano = nanoseconds[0] - sptr[position].timeSinceLastBurst_nano;
		}
	}
}

void getAction(PCB**sptr,int position, unsigned int **seconds, unsigned int **nanoseconds, int*quantum, unsigned int *waitToQueue_seconds, unsigned int *waitToQueue_nano, unsigned int *tempTimeInSystem_seconds, unsigned int *tempTimeInSystem_nano, unsigned int *tempTimeSinceLastBurst_seconds, unsigned int *tempTimeSinceLastBurst_nano){
	int randNum = (rand()%99);
        if(randNum <= 9){
		(*sptr[position]).terminating = 1;
		getUserTotalSystemTime(*sptr, position, *seconds, *nanoseconds, tempTimeInSystem_seconds, tempTimeInSystem_nano);
		getTimeSinceLastBurst( *sptr, position, *seconds, *nanoseconds, tempTimeSinceLastBurst_seconds, tempTimeSinceLastBurst_nano);
        } else if (randNum > 9 && randNum <= 39){
		getUserTotalSystemTime(*sptr, position, *seconds, *nanoseconds, tempTimeInSystem_seconds, tempTimeInSystem_nano);
                getTimeSinceLastBurst( *sptr, position, *seconds, *nanoseconds, tempTimeSinceLastBurst_seconds, tempTimeSinceLastBurst_nano);
        } else if (randNum > 39 && randNum <= 69){
		getUserTotalSystemTime(*sptr, position, *seconds, *nanoseconds, tempTimeInSystem_seconds, tempTimeInSystem_nano);
                getTimeSinceLastBurst( *sptr, position, *seconds, *nanoseconds, tempTimeSinceLastBurst_seconds, tempTimeSinceLastBurst_nano);
		(*sptr[position]).waitingToQueue = 1;
		getTimeToWait(*seconds, *nanoseconds, waitToQueue_seconds, waitToQueue_nano);
		(*sptr[position]).isSet = 0;
	} else {
		getUserTotalSystemTime(*sptr, position, *seconds, *nanoseconds, tempTimeInSystem_seconds, tempTimeInSystem_nano);
                getTimeSinceLastBurst( *sptr, position, *seconds, *nanoseconds, tempTimeSinceLastBurst_seconds, tempTimeSinceLastBurst_nano);
		randNum = ((rand()%98)+1);
		int percentTimeUsed = ((randNum / 100)**quantum);
		(*sptr[position]).totalCPUTimeUsed_nano += percentTimeUsed;
		if((*sptr[position]).totalCPUTimeUsed_nano >= 1000000000){
			(*sptr[position]).totalCPUTimeUsed_seconds++;
			(*sptr[position]).totalCPUTime_nano = (*sptr[position]).totalCPUTimeUsed_nano - 1000000000;
		}
	}
}

void getUserTotalSystemTime(PCB *sptr, int position, unsigned int *seconds, unsigned int *nanoseconds, unsigned int *tempTimeInSystem_seconds, unsigned int *tempTimeInSystem_nano){
	 if(sptr[position].totalTimeInSystem_seconds== 0 && sptr[position].totalTimeInSystem_nano == 0){
                *tempTimeInSystem_seconds = seconds[0];
                *tempTimeInSystem_nano = nanoseconds[0];
        } else {
                sptr[position].totalTimeInSystem_seconds = seconds[0] - *tempTimeInSystem_seconds;
                if(nanoseconds[0] < *tempTimeInSystem_nano){
                        sptr[position].totalTimeInSystem_nano = sptr[position].totalTimeInSystem_nano - nanoseconds[0];
                } else {
                        sptr[position].totalTimeInSystem_nano = nanoseconds[0] - sptr[position].totalTimeInSystem_nano;
       		}
	}
}

void setQuantum(PCB *sptr, int position, int *quantum, int *localQuantum){
	*quantum = 10;
	if(sptr[position].processPriority == 0){
		*localQuantum = (quantum[0]/2);
	} else {
		*localQuantum = quantum[0];
	}
};

void clearPCB(PCB*sptr, int position){
	sptr[position].totalCPUTime_seconds = 0;
        sptr[position].totalCPUTime_nano = 0;
        sptr[position].totalTimeInSystem_nano = 0;
        sptr[position].totalTimeInSystem_seconds = 0;
        sptr[position].timeSinceLastBurst_nano = 0;
        sptr[position].timeSinceLastBurst_seconds = 0;
        sptr[position].processPriority = 0;
        sptr[position].position = 0;
        sptr[position].flag = 0; 
        sptr[position].isSet = 0;
        sptr[position].terminating = 0;
        sptr[position].isRunning = 0;
	sptr[position].msgReceived = 0;
	sptr[position].waitingToQueue = 0;
}

void addUserCPUTimeToClock(PCB * sptr, int position, unsigned int* seconds, unsigned int* nanoseconds){
	int billion = 1000000000;
//	printf("User cpu seconds: %d nano: %d\n",sptr[position].totalCpuTimeSeconds, sptr[position].totalCpuTimeNano);
	if(sptr[position].totalCPUTime_seconds != 0 && sptr[position].totalCPUTime_nano != 0){
		unsigned int tempSeconds = *seconds - sptr[position].totalCPUTime_seconds;
		unsigned int tempNano = *nanoseconds - sptr[position].totalCPUTime_nano;
		*seconds += tempSeconds;
		*nanoseconds += tempNano;
		if(*nanoseconds >= billion){
			(*seconds)++;
			(*nanoseconds) = 0;
		}
	}
};

void checkForTerminatingProcesses(PCB *sptr, int *position, int *terminatingPID, int *processTerminating){
	int i;
	for (i = 0; i < 18; i++){
		if (sptr[i].terminating == 1){
			*processTerminating = 1;
			*position = i;
			*terminatingPID = sptr[i].pid;
			return; 
		}
	}
};

void howMuchQuantum(PCB *sptr, int *localQuantum, int position){
	int random = (rand()%2);
	if (random == 0){
		//sptr[position].totalCPUTime_nano += localQuantum;
	} else {
		//int randtime = (rand()%(localQuantum+1));
		//sptr[position].totalCPUTime_nano += randtime;
		*localQuantum = rand()%(*localQuantum+1);
	} 
}

void createSharedMemKeys(key_t *strctKey, key_t *semKey, key_t *timeKey, key_t *qKey){
	*strctKey = ftok(".", 'G');
	*semKey = ftok(".", 'H');
	*timeKey = ftok(".", 'C');
	*qKey = ftok(".", 'D');
};

void createSharedMemory(int *strctid, int *semid, int *timeid, int *qid, key_t strctKey, key_t semKey, key_t timeKey, key_t qKey){
	*strctid = shmget(strctKey, STRUCT_ARRAY_SIZE, 0666 | IPC_CREAT);
	*semid = shmget(semKey, SEM_SIZE, 0666 | IPC_CREAT);
	*timeid = shmget(timeKey, (sizeof(unsigned int) * 2), 0666 | IPC_CREAT);
	*qid = shmget(qKey, sizeof(int), 0666 | IPC_CREAT);
};

void attachToSharedMemory(PCB **sptr, sem_t **sem, unsigned int **seconds, unsigned int **nanoseconds, int **quantum, int semid, int strctid, int timeid, int qid){
	*sptr = (PCB *)shmat(strctid, NULL, 0);
	*sem = (sem_t *)shmat(semid, NULL, 0);
	*seconds = (unsigned int*)shmat(timeid, NULL, 0);
	*nanoseconds = *seconds + 1;
	*quantum = (int *)shmat(qid, NULL,0);
};

void initializeSemaphores(sem_t **sem){
	sem_init(*sem, 1, 1);
};

void createArgs(char *strctMem, char *semMem, char *timeMem, char *qMem, char *arrayPosition, int strctid, int semid, int timeid, int qid, int position){
	snprintf(strctMem, sizeof(strctMem)+2, "%d", strctid); 
	snprintf(semMem, sizeof(semMem)+2, "%d", semid);
	snprintf(timeMem, sizeof(timeMem)+2, "%d", timeid);
	snprintf(arrayPosition, sizeof(arrayPosition)+2, "%d", position);
	snprintf(qMem, sizeof(qMem)+2, "%d", qid);
};

void updateVector(int bitVector[]){
        int i;
	for(i = 0; i < 18; i++){
		if(bitVector[i] == 0){
			bitVector[i] = 1;
			return;
		}
	}
};

void setPriority(PCB *sptr,int position){
        int priority = rand()%9;
        if(priority == 0){
                sptr[position].processPriority = 0;
        } else {
                sptr[position].processPriority = 1;
        }
};

void initializeUser(PCB *pcbPtr, int PCBposition, unsigned int **seconds, unsigned int **nanoseconds, unsigned int *tempTimeInSystem_seconds, unsigned int *tempTimeInSystem_nano){
	srand(time(NULL));
	pcbPtr[PCBposition].pid = getpid();
	printf("USER PID %d\n", getpid());
	pcbPtr[PCBposition].isSet = 1;
	pcbPtr[PCBposition].position = PCBposition;
	//pcbPtr[PCBposition].pid = getpid();
	//pcbPtr[PCBposition].timeSinceLastBurst_seconds = seconds[0];
	//pcbPtr[PCBposition].timeSinceLastBurst_nano = nanoseconds[0];
	getUserTotalSystemTime(pcbPtr, PCBposition, *seconds, *nanoseconds, tempTimeInSystem_seconds, tempTimeInSystem_nano);
	//getUserTotalTime(pcbPtr,position, seconds, nanoseconds);
	//pcbPtr[position].totalCpuTime = 100;
	//pcbPtr[position].totalTimeAlive = 110;
	//pcbPtr[PCBposition].timeSinceLastBurstNano= 120;
	//printf("JUST SET TIME SINCE LAST BURST ASSHOLE\n");
	//pcbPtr[position].processPriority = 130;
};

void forkChild(PCB **sptr, pid_t pid, char *strctMem, char *semMem, char *timeMem, char *qMem, char *arrayPosition, int *position, unsigned int *seconds, unsigned int *nanoseconds, FILE *file){
	setPriority(*sptr, *position);
	if((pid = fork()) == 0){
		fprintf(file, "OSS: Created a user %u at %u.%u\n", getpid(), *seconds, *nanoseconds);
		fclose(file);
		execlp("./user", "./user", strctMem, semMem, timeMem, qMem, arrayPosition, NULL);
	}
	//printf("position in forkchild: %d\n", *arrayPosition);
	updateVector(bitVector);
	
};

void enqueueReadyProcess(PCB *sptr){
	int i;
	for(i = 0; i < 18; i++){
		if(sptr[i].waitingToQueue == 0){
			if(sptr[i].isSet == 1){
				if(sptr[i].processPriority == 1){
					enqueueHP(sptr, i);
			
				} else {
					enqueueLP(sptr, i);
				}	
			}
		}
	}
};

//For the queues

int isLPQueueEmpty(){
	if(headLP == NULL){
		printf("HEAD LP EMPTY\n");
		return 1;
	} else {
		printf("HEAD LP FULL\n");
		return 0;
        }
};

int isHPQueueEmpty(){
	if(headHP == NULL){
		printf("HEAD HP EMPTY\n");
		return 1;
	}else{
		printf("HEAD HP FULL\n");
		return 0;	
	}
};

int isLPQueueFull(){
        return FALSE;
};

int isHPQueueFull(){
        return FALSE;
};

void initLPQueue(){
	headLP = tailLP = NULL;
};

void initHPQueue(){
	headHP = tailHP = NULL;
};

void clearLPQueue(){
	QNode *temp;
	while(headLP != NULL){
		temp = headLP;
		headLP = headLP->next;
		free(temp);
	}
	headLP = tailLP = NULL;
};

void clearHPQueue(){
	QNode *temp;
        while(headHP != NULL){
                temp = headHP;
                headHP = headHP->next;
                free(temp);
        }
        headHP = tailHP = NULL;
};

int enqueueHP(PCB *sptr, int position){
	QNode *temp;
	if(isHPQueueFull()) return FALSE;
	printf("ENQUEUEING HP %d\n", sptr[position].pid);
	temp = (QNode *)malloc(sizeof(QNode));
	temp -> qptr = &sptr[position];
	temp -> next = NULL;
	if(headHP == NULL){
		headHP = tailHP = temp;
	} else {
		tailHP -> next = temp;
		tailHP = temp;
	}
	return TRUE; 
};

int enqueueLP(PCB *sptr, int position){
	 QNode *temp;
        if(isLPQueueFull()) return FALSE;
	printf("ENQUEUEING LP %d\n", sptr[position].pid);
        temp = (QNode *)malloc(sizeof(QNode));
        temp -> qptr = &sptr[position];
        temp -> next = NULL;
        if(headLP == NULL){
                headLP = tailLP = temp;
        } else {
                tailLP -> next = temp;
                tailLP = temp;
        }
        return TRUE;
};

pid_t dequeueHP(PCB *sptr, int *position){
	printf("DEQUEUEING HP %d\n", sptr[*position].pid);
	QNode *temp;
	if(isHPQueueEmpty()){
		return FALSE;
	} else {
		*position = headHP->qptr[0].position;
		printf("POSITIONHP: %d\n", *position);
		sptr[*position] = headHP->qptr[0];
		temp = headHP;
		headHP = headHP->next;
	
		free(temp);
		
		if(isHPQueueEmpty()){
			headHP = tailHP = NULL;
		}
	}
	return sptr[*position].pid;
};

pid_t dequeueLP(PCB *sptr, int *position){	
	printf("DEQUEUEING LP %d\n", sptr[*position].pid);
	
	QNode *temp;
        if(isLPQueueEmpty()){
                return FALSE;
        } else {
                *position = headLP->qptr[0].position;
	        printf("POSITIONLP: %d\n", *position);
		sptr[*position] = headLP->qptr[0];
		temp = headLP;
		headLP = headLP->next;
                free(temp);

                if(isLPQueueEmpty()){
                        headLP = tailLP = NULL;
                }
        }
        return sptr[*position].pid;
};
 

// end of queue funcs
