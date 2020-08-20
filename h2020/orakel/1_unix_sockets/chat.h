#ifndef _CHAT_H
#define _CHAT_H

#include <stdio.h>

#define MAX_CONNS 5 /* the maximum length for the queue of pending connections */

void client(void);
void server(void);
void handle_client(int, char *);

#endif
