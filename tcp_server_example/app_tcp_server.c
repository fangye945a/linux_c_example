#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <time.h>

#include "app_tcp_server.h"
#include "socket_server.h"

/*TCP线程ID*/
static pthread_t ulAPPTCPThreadID;
static int TCP_isrunning_flag = 0;

/*解析APP发来的JSON请求*/
void parse_app_data(int clientFd, char *request)
{
	
}

/*TCP已连接回调*/
void server_connectCB(int clientFd)
{
	printf("server_connectCB++[%d]\n", clientFd);
}


/*TCP接收数据包回调*/
void server_rxCB(int clientFd)   //change by fangye 20180806
{
  	char buffer[MAX_MESSAGE_LEN] = {0};  //单个json对象最大长度256
	int byteToRead = 0;
	int buff_index = 0;
	int flag = 0;  //记录{}标志
	int rtn = -1;
	char ch = 0;  //记录临时字符
	rtn = ioctl(clientFd, FIONREAD, &byteToRead);
	if (rtn != 0)
	{
		printf("server_rxCB: Socket error\n");
	}
	else
	{
		printf("Socket Recieve %d bytes pakage\n", byteToRead);
	}

	while (byteToRead)  //循环读取数据
	{
		ch = 0;
		read(clientFd, &ch, 1); //读一个字节
		if (ch == '{') //遇到json对象
		{
			flag++;

		}
		else if (ch == '}' && flag > 0)
		{
			flag--;
		}

		if (flag == 0) //若没有遇到json对象或者已读完一个json对象
		{
			if (buff_index > 8 && buff_index < MAX_MESSAGE_LEN-1) //判断是否有有效数据,json对象长度至少为9bytes才做处理 {"a":"b"}
			{
				buffer[buff_index++] = ch; //读取数据
				parse_app_data(clientFd, buffer); //解析数据
				printf("parse object: %s\n", buffer);
				memset(buffer, 0, buff_index);
				buff_index = 0;  //下标清零
			}
		}
		else
		{
			if (buff_index < MAX_MESSAGE_LEN-1) //json对象超出长度
				buffer[buff_index++] = ch; //读取数据
			else  //丢掉该包超过长度的数据
			{
				buff_index = 0;
				bzero(buffer, sizeof(buffer));
				while (flag == 0 || byteToRead == 0)
				{
					read(clientFd, &ch, 1); //读一个字节
					if (ch == '{') //遇到json对象
					{
						flag++;

					}
					else if (ch == '}')
					{
						flag--;
					}
					byteToRead--;
				}

			}
		}
		byteToRead--;
	}
	return;
}

/*初始化TCPServer*/
void ServerInit(void)
{
	if (socketServerInit(TCP_PORT) == -1)
	{
		printf("TCP Server init fail....................");
		return;
	}

	socketServerConfig(server_rxCB, server_connectCB);
}

/*TCPServer处理线程*/
void *AppTcpThread(void* arg)
{
	printf("TCP Server thread start ....................");

	ServerInit();

	while (TCP_isrunning_flag)
	{
		int numClientFds = socketServerGetNumClients();

		//poll on client socket fd's for any activity
		if (numClientFds)
		{
			int pollFdIdx;
			int *client_fds = malloc(numClientFds * sizeof(int));

			//socket client FD's
			struct pollfd *pollFds = malloc((numClientFds * sizeof(struct pollfd)));

			if (client_fds && pollFds)
			{
				//Set the socket file descriptors
				socketServerGetClientFds(client_fds, numClientFds);
				for (pollFdIdx = 0; pollFdIdx < numClientFds; pollFdIdx++)
				{
					pollFds[pollFdIdx].fd = client_fds[pollFdIdx];
					pollFds[pollFdIdx].events = POLLIN | POLLRDHUP;
				}
				
				poll(pollFds, numClientFds, -1);
				for (pollFdIdx = 0; pollFdIdx < numClientFds; pollFdIdx++)
				{
					if ((pollFds[pollFdIdx].revents))
					{
						socketServerPoll(pollFds[pollFdIdx].fd,
								pollFds[pollFdIdx].revents);
					}
				}

				free(client_fds);
				free(pollFds);
			}
		}
	}
	return NULL;
}


void send_data_to_client(char *data , int len)
{
	int numClientFds = socketServerGetNumClients();					//获取客户端个数
	int *client_fds = malloc(numClientFds * sizeof(int));
	if(client_fds)
	{
		socketServerGetClientFds(client_fds, numClientFds);			//获取客户端文件描述符
		int i=0;
		for(i=1; i<numClientFds; i++)	//不发送给第一个连接
		{
			socketServerSend(data, len, client_fds[i]);	//发送数据到客户端
		}
		free(client_fds);
		client_fds = NULL;
	}
}


int app_tcp_init(void)
{
	TCP_isrunning_flag = 1;
	/*创建TCP线程*/
	if (0 != pthread_create(&ulAPPTCPThreadID, NULL, AppTcpThread, NULL))
	{
		printf("Create APPTCPThread failed!");
		return -1;
	}
	return 0;
}

void app_tcp_destroy(void)
{
	/*等待线程退出*/
	TCP_isrunning_flag = 0;
	
	if (ulAPPTCPThreadID != 0)
	{
		pthread_cancel(ulAPPTCPThreadID); //解决线程阻塞导致无法回收问题
		pthread_join(ulAPPTCPThreadID, NULL);
	}
}
