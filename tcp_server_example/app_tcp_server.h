#ifndef __APP_TCP_SERVER__
#define __APP_TCP_SERVER__

/*TCPServer端口号*/
#define TCP_PORT 10086
#define MAX_MESSAGE_LEN 2048


/*模块初始化*/
extern int app_tcp_init(void);

/*释放资源(调用的pthread_jion)*/
extern void app_tcp_destroy(void);

#endif //__APP_TCP_SERVER__
