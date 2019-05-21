/*******************************************************************************
 * Copyright (C) 2017 ZOOMLION
 *
 * All rights reserved.
 *
 * Contributors:
 *      jea.long    initial version
 *
 *******************************************************************************/
/**
 * @file socket_server.h
 *
 */

#ifndef SOCKET_SERVER_H
#define SOCKET_SERVER_H

# define POLLRDHUP	0x2000

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * TYPEDEFS
 */

typedef void (*socketServerCb_t)( int clientFd );

/**
 * INCLUDES
 */
#include <stdint.h>
//#include "hal_types.h"

/**
 * CONSTANTS
 */

#define MAX_CLIENTS 5

/*
 * serverSocketInit - initialises the server.
 */
int32_t socketServerInit( uint32_t port );

/*
 * serverSocketConfig - initialises the server.
 */
int32_t socketServerConfig(socketServerCb_t rxCb, socketServerCb_t connectCb);

/*
 * getClientFds -  get clients fd's.
 */
void socketServerGetClientFds(int *fds, int maxFds);

/* 
 * getClientFds - get clients fd's.
 */
uint32_t socketServerGetNumClients(void);

/*
 * socketSeverPoll - services the Socket events.
 */
void socketServerPoll(int clinetFd, int revent);

/*
 * socketSeverSendAllclients - Send a buffer to all clients.
 */
int32_t socketServerSendAllclients(uint8_t* buf, uint32_t len);

/*
 * socketSeverSend - Send a buffer to a clients.
 */
int32_t socketServerSend(uint8_t* buf, uint32_t len, int32_t fdClient);


/*
 * socketSeverClose - Closes the client connections.
 */
void socketServerClose(void);


#ifdef __cplusplus
}
#endif

#endif /* SOCKET_SERVER_H */
