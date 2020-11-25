// 关于socket编程函数详解见：https://blog.csdn.net/hguisu/article/details/7445768?ops_request_misc=%257B%2522request%255Fid%2522%253A%2522160619074219724842951464%2522%252C%2522scm%2522%253A%252220140713.130102334..%2522%257D&request_id=160619074219724842951464&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~sobaiduend~default-2-7445768.pc_first_rank_v2_rank_v28&utm_term=linux中socket编程&spm=1018.2118.3001.4449

#include "string.h"
#include "fcntl.h"
#include "sys/wait.h"
#include "stdio.h"
#include "stdlib.h"
#include "sys/types.h"
#include "unistd.h"
#include "sys/stat.h"
#include "netinet/in.h"
#include "clientmsg.h"
#include "servermsg.h"
#include "semaphore.h"

// QUEUE结构体
struct QUEUE
{
	int qid;
	int stat;
};

// 传入两个字符串，建立管道
void mkfifo_fun(char server_fifo[], char client_fifo[])
{
	// unlink()会删除参数（指针）指定的文件. 如果该文件名为最后连接点, 但有其他进程打开了此文件,
	// 则在所有关于此文件的文件描述词皆关闭后才会删除. 如果参数pathname 为一符号连接, 则此连接会被删除。
	// 为了后面建立管道
	unlink(server_fifo);
	unlink(client_fifo);

	// 定义函数 int mkfifo(const char * pathname,mode_t mode);
	// mkfifo()会依参数pathname建立特殊的FIFO文件，该文件必须不存在，而参数mode为该文件的权限（mode%~umask）
	// 建立实名管道
	mkfifo(server_fifo, 0666);
	mkfifo(client_fifo, 0666);
};

void write_to_client_fifo_fun(int fifo_write, struct CLIENTMSG C_recvmsg, struct sockaddr_in client, int qid)
{

	struct SERVERMSG S_sendmsg;
	S_sendmsg.OP = C_recvmsg.OP;
	strcpy(S_sendmsg.username, C_recvmsg.username);
	strcpy(S_sendmsg.buf, C_recvmsg.buf);
	S_sendmsg.client = client;
	S_sendmsg.qid = qid;
	write(fifo_write, &S_sendmsg, sizeof(S_sendmsg));
}

int main()
{
	//1. 创建并初始化变量
	// struct 结构名 对象名
	struct QUEUE queue[5];

	int serverfd, clientfd;
	// 地址结构
	struct sockaddr_in server, client;

	// 建立管道（一种特殊文件）
	mkfifo_fun("SERVER", "CLIENT");

	// RDWR表示读写模式，open函数成功则放回文件描述符
	// 个人看法：针对read和write可以分别使用只读模式和只写模式，防止出错
	int server_fifo_read = open("SERVER", O_RDWR);
	int server_fifo_write = open("SERVER", O_RDWR);
	int client_fifo_read = open("CLIENT", O_RDWR);
	int client_fifo_write = open("CLIENT", O_RDWR);

	// 2. 初始化server地址结构+创建socket+绑定+监听

	// 设置地址信息前先清0
	bzero(&server, sizeof(&server));
	//INADDR_ANY就是指定地址为0.0.0.0的地址，这个地址事实上表示不确定地址，或“所有地址”、“任意地址”。
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(1234);
	// 理论上建立socket时是指定协议，应该用PF_xxxx，设置地址时应该用AF_xxxx。可以混用，但是不规范
	server.sin_family = AF_INET;
	// 获取长度
	socklen_t len = sizeof(server);
	// 建立socket描述符
	serverfd = socket(AF_INET, SOCK_STREAM, 0);
	// 绑定描述符和地址结构，并开始监听
	bind(serverfd, (struct sockaddr *)&server, len);
	// listen函数的第一个参数即为要监听的socket描述字，
	// 第二个参数为相应socket可以排队的最大连接个数。
	// socket()函数创建的socket默认是一个主动类型的，listen函数将socket变为被动类型的，等待客户的连接请求。
	listen(serverfd, 6);

// ？
	//3. create semid 创建信号标识符
	// 程序对所有信号量的访问都是间接的，程序先通过调用semget()函数并提供一个键，
	// 再由系统生成一个相应的信号标识符semid（semget()函数的返回值）
	// 只有semget()函数才直接使用信号量键，所有其他的信号量函数使用由semget()函数返回的信号量标识符semid。
	// 如果多个程序使用相同的key值，key将负责协调工作。

	// 系统建立IPC通讯必须指定一个ID值。通常情况下，该id值通过ftok函数得到。
	key_t key;
	key = ftok(".", 'a');
	int semid = CreateSem(key, 5);

	//4. create fork()
	int pid = fork();
	// 创建失败
	if (pid < 0)
	{
		perror("main fork error");
		exit(1);
	}
	// 返回到新建的子进程
	else if (pid == 0) //转发子进程  从命名管道CLIENT中读取通信子进程发来的消息，消息类型为：用户名、退出及一般信息；
	{
		printf("####create 5  msg quenence\n");
		struct SERVERMSG S_recvmsg;
		int i = 0;
		printf("####");
		fflush(NULL);

		for (i; i < 5; i++)
		{
			key_t key = ftok(".", i);

			if (-1 == (queue[i].qid = msgget(key, IPC_CREAT | IPC_EXCL | 0666)))
			{
				msgctl(msgget(key, IPC_CREAT | 0666), IPC_RMID, NULL);
				queue[i].qid = msgget(key, IPC_CREAT | IPC_EXCL | 0666);
			}
			queue[i].stat = 0;
			printf("    %d", queue[i].qid);
			fflush(NULL);
		}
		printf("\n");

		write(server_fifo_write, queue, sizeof(queue));
		while (1)
		{

			if (-1 == read(client_fifo_read, &S_recvmsg, sizeof(S_recvmsg)))
			{
				perror("read client error\n");
				exit(1);
			}
			struct MESSAGE message;
			int tmp;
			switch (S_recvmsg.OP)
			{

			case USER: //若为用户名，依据消息队列在更新客户信息表，状态为可用；
				message.msgtype = S_recvmsg.qid;
				message.msg = S_recvmsg;
				//printf("#####user is coming:%s, clietn left %d \n", message.msg.username, GetvalueSem(semid));
				//依据消息队列在更新客户信息表，状态为可用
				for (tmp = 0; tmp < 5; tmp++)
				{
					if (queue[tmp].qid == message.msg.qid)
					{
						queue[tmp].stat = 1;
					}
					else if (queue[tmp].stat == 1)
					{
						msgsnd(queue[tmp].qid, &message, sizeof(message), 0);
					}
				}
				write(server_fifo_write, queue, sizeof(queue));
				break;

			case MSG: //若为一般信息，将信息转换后写入可用客户的消息队列，等待其他通信子进程读取；

				message.msgtype = S_recvmsg.qid;
				message.msg = S_recvmsg;
				for (tmp = 0; tmp < 5; tmp++)
				{
					if (queue[tmp].qid != message.msg.qid && queue[tmp].stat == 1)
					{
						if (-1 == (msgsnd(queue[tmp].qid, &message, sizeof(message), 0)))
						{
							perror("error msgsnd");
							exit(1);
						}
					}
				}
				break;

			case EXIT: //若为退出，在客户信息表中状态设为不可用，执行信号量V操作，并将可用客户的消息队列标识符写入到命名管道SERVER；

				message.msgtype = S_recvmsg.qid;
				message.msg = S_recvmsg;

				for (tmp = 0; tmp < 5; tmp++)
				{
					if (queue[tmp].stat == 1)
					{
						if (-1 == (msgsnd(queue[tmp].qid, &message, sizeof(message), 0)))
						{
							perror("error msgsnd");
							exit(1);
						}
					}
					if (queue[tmp].qid == message.msg.qid)
					{
						queue[tmp].stat = 0;
					}
				}

				struct QUEUE clear[5];
				read(server_fifo_read, &clear, sizeof(clear));
				write(server_fifo_write, queue, sizeof(queue));
				break;

			default:
				break;
			}
			S_recvmsg.OP = 100;
		}
	}
	// 返回到主进程
	else 
	{
		while (1)
		{

			clientfd = accept(serverfd, (struct sockaddr *)&client, &len);

			// CLIENTMSG在clientmsg.h中声明
			struct CLIENTMSG C_recvmsg, C_sendmsg;

			// 函数Sem_V在semaphore.h中声明
			if (0 == Sem_V(semid))
			{
				printf("#### new client  PORT: %d    IP:%s \n", ntohs(client.sin_port), inet_ntoa(client.sin_addr.s_addr));

				C_sendmsg.OP = OK;
				strcpy(C_sendmsg.username, "OK");
				strcpy(C_sendmsg.buf, "OK");
				write(clientfd, &C_sendmsg, sizeof(C_sendmsg));

				int id = fork(); //创建   主进程/通信子进程
				if (id < 0)
				{
					perror("id error");
					exit(1);
				}
				else if (id == 0) //主进程/通信子进程
				{
					int qid;
					struct QUEUE recvqueue[5];
					read(server_fifo_read, &recvqueue, sizeof(recvqueue));
					int tep = 0;
					for (tep; tep < 5; tep++)
					{
						if (recvqueue[tep].stat == 0)
						{
							qid = recvqueue[tep].qid;
							break;
						}
					}

					int recvid = fork(); //创建   主进程/通信子进程/接收进程

					if (recvid < 0)
					{
						perror("recvid error");
						exit(1);
					}
					else if (recvid == 0) //主进程/通信子进程/子进程  -创建一个子进程负责从消息队列中读取消息，发送给客户
					{
						while (1)
						{
							struct SERVERMSG S_msg;
							struct MESSAGE recvmsg;
							if (-1 == (msgrcv(qid, &recvmsg, sizeof(recvmsg), 0, 0)))
							{
								perror("msgrcv error");
								exit(1);
							}

							S_msg = recvmsg.msg;
							strcpy(C_sendmsg.username, S_msg.username);
							strcpy(C_sendmsg.buf, S_msg.buf);

							if (S_msg.OP == EXIT && S_msg.qid == qid)
							{

								Sem_P(semid);
								printf("#### this name:%s pid :%d exit  client left :%d !!\n", S_msg.username, qid, GetvalueSem(semid));
								C_sendmsg.OP = S_msg.OP;
								write(clientfd, &C_sendmsg, sizeof(C_sendmsg));
								break;
							}
							else if (S_msg.OP == USER)
							{
								C_sendmsg.OP = S_msg.OP;
							}
							else if (S_msg.OP == MSG || S_msg.OP == EXIT)
							{
								C_sendmsg.OP = MSG;
							}

							write(clientfd, &C_sendmsg, sizeof(C_sendmsg));
						}
					}
					else //主进程/通信子进程// 接收客户端数据  -通过CLIENT发送给转发子进程
					{

						while (1)
						{

							if (-1 == read(clientfd, &C_recvmsg, sizeof(C_recvmsg))) //若信息为退出，终止子进程，程序结束
							{
								perror("error read");
								exit(1);
							}
							if (C_recvmsg.OP == -100)
							{
								C_recvmsg.OP = EXIT;
								printf("####CLIENT BREAKDOWN WHEN COMMUNICATION  -->%d !!!!\n", C_recvmsg.OP);
								write_to_client_fifo_fun(client_fifo_write, C_recvmsg, client, qid);
							}

							if (C_recvmsg.OP == EXIT)
							{

								write_to_client_fifo_fun(client_fifo_write, C_recvmsg, client, qid);
								break;
							}
							else if (C_recvmsg.OP == USER) //若信息为用户名，附带消息队列、客户地址发送给转发子进程；
							{

								printf("#####user is coming:%s, clietn left %d \n", C_recvmsg.username, GetvalueSem(semid));
								write_to_client_fifo_fun(client_fifo_write, C_recvmsg, client, qid);
							}
							else if (C_recvmsg.OP == MSG) //若信息为用户名，附带消息队列、客户地址发送给转发子进程；
							{

								write_to_client_fifo_fun(client_fifo_write, C_recvmsg, client, qid);
							}

							C_recvmsg.OP = -100;
						}

						waitpid(recvid, NULL, WNOHANG);
						close(clientfd);
					}
				}
				else //主进程
				{

					close(clientfd);
					waitpid(id, NULL, WNOHANG);
				}
			}
			else //超过5个
			{

				C_sendmsg.OP = EXIT;
				strcpy(C_sendmsg.username, "server");
				strcpy(C_sendmsg.buf, "over client");
				write(clientfd, &C_sendmsg, sizeof(C_sendmsg));
				close(clientfd);
			}
		}
	}

	return 0;
}
