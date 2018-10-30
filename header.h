#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <time.h>
#include <stdlib.h>

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
	int totalCpuTime;
	int totalTimeAlive;
	int timeSinceLastBurst;
	int processPriority;
	int position;
	int flag;
	int isSet;
	int terminating;
	pid_t pid;
}PCB;

PCB pcbArray[18];
PCB *sptr = NULL;
sem_t *sem = NULL;
pid_t pid = 0;
key_t strctKey = 0, semKey = 0, timeKey = 0, posKey = 0, qKey = 0;
unsigned int *seconds = 0, *nanoseconds = 0, *randsecs = 0, *randnanosecs = 0;
int *quantum = 0; 
static int headHP, tailHP, headLP, tailLP;
static pid_t theLPQueue[QUEUE_SIZE], theHPQueue[QUEUE_SIZE];
int strctid = 0, semid = 0, timeid = 0, qid = 0,position = 0, bitVector[18];
char strctMem[10], semMem[10], timeMem[10], qMem[10], arrayPosition[10];

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
	*seconds = (unsigned int*)shmat(semid, NULL, 0);
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

void updateVector(int bitVector[], int *isFull){
        bitVector[position] = 1;
};

void forkChild(pid_t pid, char *strctMem, char *semMem, char *timeMem, char *qMem, char *arrayPosition, int *position){
	if((pid = fork()) == 0){
		execlp("./user", "./user", strctMem, semMem, timeMem, qMem, arrayPosition, NULL);
	}
	updateVector(bitVector, position);
};

void setPriority(PCB *sptr,int position){
	int priority = rand()%11; 	
	if(priority == 0){
		sptr[position].processPriority = 0;
	} else {
		sptr[position].processPriority = 1;
	}
};

//For the queues

void initLPQueue();
void initHPQueue();

void clearHPQueue();
void clearLPQueue();

int enqueueHP(PCB *sptr, int position);
int enqueueLP(PCB *sptr, int position);

pid_t dequeueHP(PCB *sptr);
pid_t dequeueLP(PCB *sptr);

int isLPQueueEmpty();
int isHPQueueEmpty();

int isLPQueueFull();
int isHPQueueFull();

// end of queue funcs
