#ifndef _HTTPSERVER_H_
#define _HTTPSERVER_H_

#include <stddef.h>

#define NET_SOFTERROR -1
#define NET_HARDERROR -2


int           poll_fd(int, char, int);
int           Nwrite(int, char *, size_t);
unsigned long parse_get_request(char *, char *);
void          set_nonblocking(int);
void          upload(int, char *, unsigned long);


#endif // #ifndef _HTTPSERVER_H_
