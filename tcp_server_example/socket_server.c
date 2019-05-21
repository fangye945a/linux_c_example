#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <errno.h>
#include <stdint.h>
#include "socket_server.h"


/**
 * TYPEDEFS
 */
typedef struct
{
    void   *next;
    int socketFd;
    socklen_t cli_len;
    struct sockaddr_in cli_addr;
} socketRecord_t;


/**
 * GLOBAL VARIABLES
 */
socketRecord_t *socketRecordHead = NULL;

socketServerCb_t socketServerRxCb;
socketServerCb_t socketServerConnectCb;

/**
 * @fn      createSocketRec
 * @brief   create a socket and add a rec fto the list.
 * @param   table
 * @param   rmTimer
 * @return  new clint fd
 */
int createSocketRec( void  )
{
    int tr=1;
    socketRecord_t *srchRec;

    socketRecord_t *newSocket = malloc( sizeof( socketRecord_t ) );

    //open a new client connection with the listening socket (at head of list)
    newSocket->cli_len = sizeof(newSocket->cli_addr);

    //Head is always the listening socket
    newSocket->socketFd = accept(socketRecordHead->socketFd,
                                 (struct sockaddr *) &(newSocket->cli_addr),
                                 &(newSocket->cli_len));

    if (newSocket->socketFd < 0)
        printf("accept error!");
           

    // Set the socket option SO_REUSEADDR to reduce the chance of a
    // "Address Already in Use" error on the bind
    setsockopt(newSocket->socketFd,SOL_SOCKET,SO_REUSEADDR,&tr,sizeof(int));
    // Set the fd to none blocking
    fcntl(newSocket->socketFd, F_SETFL, O_NONBLOCK);

    newSocket->next = NULL;

    //find the end of the list and add the record
    srchRec = socketRecordHead;
    // Stop at the last record
    while ( srchRec->next )
        srchRec = srchRec->next;

    // Add to the list
    srchRec->next = newSocket;

    return(newSocket->socketFd);
}

/**
 * @fn      deleteSocketRec
 * @brief   Delete a rec from list.
 * @param   table
 * @param   rmTimer
 * @return  none
 */
void deleteSocketRec( int rmSocketFd )
{
    socketRecord_t *srchRec, *prevRec=NULL;

    // Head of the timer list
    srchRec = socketRecordHead;

    // Stop when rec found or at the end
    while ( (srchRec->socketFd != rmSocketFd) && (srchRec->next) )
    {
        prevRec = srchRec;
        // over to next
        srchRec = srchRec->next;
    }

    if (srchRec->socketFd != rmSocketFd)
    {
        printf("deleteSocketRec: record not found");
        return;
    }

    // Does the record exist
    if ( srchRec )
    {
        // delete the timer from the list
        if ( prevRec == NULL )
        {
            //trying to remove first rec, which is always the listining socket
            printf("deleteSocketRec: removing first rec, which is always the listining socket.");
            return;
        }

        //remove record from list
        prevRec->next = srchRec->next;

        close(srchRec->socketFd);
        free(srchRec);
    }
}

/**
 * @fn      serverSocketInit
 * @brief   initialises the server.
 * @param
 * @return  Status
 */
int32_t socketServerInit( uint32_t port )
{
    struct sockaddr_in serv_addr;
    int stat, tr=1;

    if(socketRecordHead == NULL)
    {
        // New record
        socketRecord_t *lsSocket = malloc( sizeof( socketRecord_t ) );

        lsSocket->socketFd = socket(AF_INET, SOCK_STREAM, 0);
        if (lsSocket->socketFd < 0)
        {
            printf("opening socket fail");
            return -1;
        }

        // Set the socket option SO_REUSEADDR to reduce the chance of a
        // "Address Already in Use" error on the bind
        setsockopt(lsSocket->socketFd, SOL_SOCKET, SO_REUSEADDR, &tr, sizeof(int));
        // Set the fd to none blocking
        fcntl(lsSocket->socketFd, F_SETFL, O_NONBLOCK);

        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(port);
        stat = bind(lsSocket->socketFd, (struct sockaddr *) &serv_addr,
                    sizeof(serv_addr));
        if ( stat < 0)
        {
            printf("binding error: %s\n", strerror( errno ) );
            return -1;
        }
        //will have MAX_CLIENTS pending open client requests
        listen(lsSocket->socketFd, MAX_CLIENTS);

        lsSocket->next = NULL;
        //Head is always the listening socket
        socketRecordHead = lsSocket;
    }
	
    return 0;
}

/**
 * @fn      serverSocketConfig
 * @brief   register the Rx Callback.
 * @param
 * @return  Status
 */
int32_t socketServerConfig(socketServerCb_t rxCb, socketServerCb_t connectCb)
{
    socketServerRxCb = rxCb;
    socketServerConnectCb = connectCb;

    return 0;
}
/**
 * @fn      socketSeverGetClientFds()
 * @brief   get clients fd's.
 * @param   none
 * @return  list of Timerfd's
 */
void socketServerGetClientFds(int *fds, int maxFds)
{  
    uint32_t recordCnt=0;
    socketRecord_t *srchRec;

    // Head of the timer list
    srchRec = socketRecordHead;
    
    // Stop when at the end or max is reached
    while ( (srchRec) && (recordCnt < maxFds) )
    {
        fds[recordCnt++] = srchRec->socketFd;

        srchRec = srchRec->next;
    }

    return;
}

/**
 * @fn      socketSeverGetNumClients()
 * @brief   get clients fd's.
 * @param   none
 * @return  list of Timerfd's
 */
uint32_t socketServerGetNumClients(void)
{  
    uint32_t recordCnt=0;
    socketRecord_t *srchRec;

    // Head of the timer list
    srchRec = socketRecordHead;

    if(srchRec==NULL)
    {
        printf("socketSeverGetNumClients: socketRecordHead NULL\n");
        return -1;
    }
    
    // Stop when rec found or at the end
    while ( srchRec )
    {
        srchRec = srchRec->next;
        recordCnt++;
    }

   // printf("socketSeverGetNumClients %d", recordCnt);
    return (recordCnt);
}


/**
 * @fn      socketSeverPoll()
 * @brief   services the Socket events.
 * @param   clinetFd - Fd to services
 * @param   revent - event to services
 * @return  none
 */
void socketServerPoll(int clientFd, int revent)
{
    //is this a new connection on the listening socket
    if(clientFd == socketRecordHead->socketFd)
    {
        int newClientFd = createSocketRec();

        if(socketServerConnectCb)
        {
            socketServerConnectCb(newClientFd);
        }
    }
    else
    {
        //this is a client socket is it a input or shutdown event
        if (revent & POLLIN)
        {
            //its a Rx event
            if(socketServerRxCb)
            {
                socketServerRxCb(clientFd);
            }

        }
        if (revent & POLLRDHUP)
        {
            //its a shut down close the socket
            printf("Client fd:%d disconnected\n", clientFd);

            //remove the record and close the socket
            deleteSocketRec(clientFd);
        }
    }


    //write(clientSockFd,"I got your message",18);

    return;
}

/**
 * @fn      socketSeverSend
 * @brief   Send a buffer to a clients.
 * @param   uint8* srpcMsg - message to be sent
 *          int32 fdClient - Client fd
 * @return  Status
 */
int32_t socketServerSend(uint8_t* buf, uint32_t len, int32_t fdClient)
{ 
    int32_t rtn;
    if(fdClient)
    {
        rtn = write(fdClient, buf, len);
        if (rtn < 0)
        {
            printf("write date to fdClient %d error!", fdClient);
            return rtn;
        }
    }
    return 0;
}  


/**
 * @fn      socketSeverSendAllclients
 * @brief   Send a buffer to all clients.
 * @param   uint8* srpcMsg - message to be sent
 * @return  Status
 */
int32_t socketServerSendAllclients(uint8_t* buf, uint32_t len)
{ 
    int rtn;
    socketRecord_t *srchRec;
    srchRec = socketRecordHead->next;
    
    // Stop when at the end or max is reached
    while ( srchRec )
    {
        rtn = write(srchRec->socketFd, buf, len);
        if (rtn < 0)
        {
			printf("write date to socket %d error!", srchRec->socketFd);
			printf("closing client socket");
			
            deleteSocketRec(srchRec->socketFd);
            return rtn;
        }
        srchRec = srchRec->next;
    }
    
    return 0;
}

/**
 * @fn      socketSeverClose
 * @brief   Closes the client connections.
 * @return  Status
 */
void socketServerClose(void)
{
    int fds[MAX_CLIENTS], idx=0;

    socketServerGetClientFds(fds, MAX_CLIENTS);

    while(socketServerGetNumClients() > 1)
    {		
		printf("socketSeverClose: Closing socket fd:%d\n", fds[idx]);
        deleteSocketRec( fds[idx++] );
    }
    
    //Now remove the listening socket
    if(fds[0])
    {
        printf("socketSeverClose: Closing the listening socket.");
        close(fds[0]);
    }
}

