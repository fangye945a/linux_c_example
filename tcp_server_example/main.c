#include <stdio.h>
#include <unistd.h>
#include "app_tcp_server.h"
#include "socket_server.h"

int main(int argc, char *argv[])
{
	app_tcp_init();	//初始化tcp服务程序

	while(1)
	{
		sleep(1);
	}
	
	app_tcp_destroy(); //释放tcp服务程序
	
	return 0;
}
