#ifndef _COMMON_H
#define _COMMON_H

#include <time.h>

#define SOCKET_NAME "server_unix_path"
#define HELLO_INTERVAL 5000
#define HELLO_TIMEOUT 10000
#define MAX_EVENTS 5
#define MAX_CONNS 5

enum state {
    INIT = 0,
    WAIT,
    EXIT
};

double diff_time_ms(struct timespec, struct timespec);
int setup_client_sock(void);
int setup_server_sock(void);

#endif
