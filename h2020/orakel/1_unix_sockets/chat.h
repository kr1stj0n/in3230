/*
 * File: chat.h
 */

#ifndef _CHAT_H
#define _CHAT_H

#define SOCKET_NAME "server.socket"
#define MAX_CONNS 5 /* maximum length of the pending connections queue */

void client(void);
void server(void);
void handle_client(int, char *);

#endif
