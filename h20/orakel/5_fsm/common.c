#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>				/* memset */
#include <sys/socket.h>         /* sockets operations */
#include <sys/un.h>             /* definitions for UNIX domain sockets */
double diff_time_ms(struct timespec start, struct timespec end)
{
    double s, ms, ns;

    s  = (double)end.tv_sec  - (double)start.tv_sec;
    ns = (double)end.tv_nsec - (double)start.tv_nsec;

    if (ns < 0) { // clock underflow
        --s;
        ns += 1000000000;
    }

    ms = ((s) * 1000 + ns/1000000.0);

    return ms;
}

int setup_client_sock(void)
{
    struct sockaddr_un addr;
    int sock, rc;

    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
    }
    
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_NAME, sizeof(addr.sun_path) - 1);

    rc = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
    if ( rc < 0) {
    	perror("connect");
		close(sock);
    	exit(EXIT_FAILURE);
    }

    return sock;
}

int setup_server_sock(void)
{
    struct sockaddr_un addr;
    int sock, rc;

	/* 1. Create UNIX socket */
    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
    }

    memset(&addr, 0, sizeof(struct sockaddr_un));

    /* 2. Bind socket to socket name. */

    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_NAME, sizeof(addr.sun_path) - 1);

    /* Unlink the socket so that we can reuse the program.
     * This is bad hack! Better solution with a lock file,
     * or signal handling.
     * Check https://gavv.github.io/articles/unix-socket-reuse
     */
    unlink(SOCKET_NAME);

    rc = bind(sock, (const struct sockaddr *)&addr, sizeof(addr));
    if (rc == -1) {
        perror("bind");
        close(sock);
        exit(EXIT_FAILURE);
    }

    /*
     * 3. Prepare for accepting incomming connections.
     * The backlog size is set to MAX_CONNS.
     * So while one request is being processed other requests
     * can be waiting.
     */

    rc = listen(sock, MAX_CONNS);
    if (rc == -1) {
        perror("listen()");
        close(sock);
        exit(EXIT_FAILURE);
    }

    return sock;
}
