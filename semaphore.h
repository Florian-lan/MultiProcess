// 关于信号量和相关函数参考链接 https://www.cnblogs.com/52php/p/5851570.html

#ifndef _semaphore
#define _semaphore
#include "sys/sem.h"
#include "stdio.h"
#include "stdlib.h"

// 新建共用结构体
union semunt
{
	int val;
	struct semid_ds *buf;
	unsigned short *array;
};

// 创建信号量，返回信号标识符

int CreateSem(key_t key, int value)
{
	union semunt sem;
	int semid;
	sem.val = value;
	if (-1 == (semid = semget(key, 1, IPC_CREAT)))
	{
		// 在stdio.h头文件中
		perror("semget error");
		// 在stdlib.h中
		exit(1);
	}
	semctl(semid, 0, SETVAL, sem);
	return semid;
};

int Sem_P(int semid)
{
	struct sembuf sops = {0, +1, IPC_NOWAIT};
	return (semop(semid, &sops, 1));
};

int Sem_V(int semid)
{
	struct sembuf sops = {0, -1, IPC_NOWAIT};
	return (semop(semid, &sops, 1));
};

int GetvalueSem(int semid)
{
	union semunt sem;
	return semctl(semid, 0, GETVAL, sem);
}
void DestroySem(int semid)
{

	union semunt sem;
	sem.val = 0;
	semctl(semid, 0, IPC_RMID, sem);
}
#endif
