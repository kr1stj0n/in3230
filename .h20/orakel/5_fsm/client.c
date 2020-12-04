#include <stdio.h>		/* standard input/output library functions */
#include <stdlib.h>             /* standard library definitions (macros) */
#include <unistd.h>             /* standard symbolic constants and types */
#include <string.h>             /* string operations (strncpy, memset..) */
#include <sys/epoll.h>          /* epoll */

#include "common.h"

int main()
{
    enum state cln_state;
    struct timespec lastHelloSent;
    struct timespec lastHelloRecv;
    struct timespec timenow;
    struct epoll_event ev, events[MAX_EVENTS];
    char hello[6];
    int sock, epollfd, rc;

    sock = setup_client_sock();

    /* Setup epoll() */
    epollfd = epoll_create1(0);
    if (epollfd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }
    ev.events = EPOLLIN;
    ev.data.fd = sock;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sock, &ev) == -1) {
        perror("epoll_ctl: sock");
        exit(EXIT_FAILURE);
    }
    /* End of Setup epoll() */

    cln_state = INIT;

    while (1) {
        switch (cln_state) {
            case INIT:
                printf("sending hello\n");
                rc = write(sock, "hello", 6);
                clock_gettime(CLOCK_REALTIME, &lastHelloSent);
                cln_state = WAIT;
                break;
            case WAIT:
                rc = epoll_wait(epollfd, events, MAX_EVENTS, 1000);
                if (rc == -1) {
                    perror("epoll_wait");
                    exit(EXIT_FAILURE);
                } else if (rc == 0) {
                    clock_gettime(CLOCK_REALTIME, &timenow);
                    if (diff_time_ms(lastHelloRecv, timenow) > HELLO_TIMEOUT) {
                        /* Timeout expired and didn't get any Hello */ 
                        cln_state = EXIT;
                        continue;
                    } else {
                        if (diff_time_ms(lastHelloSent, timenow) >= HELLO_INTERVAL) {
                            /* Time to send HELLO */
                            cln_state = INIT;
                            continue;
                        }
                    }
                } else {
                    /* epoll() was triggered -> read the HELLO */
                    if (read(sock, hello, 6 ) <= 0)
                        cln_state = EXIT;
                    else {
                        printf("received hello\n");
                        clock_gettime(CLOCK_REALTIME, &lastHelloRecv);
                    }
                }
                break;
            case EXIT:
                close(sock);
                exit(EXIT_SUCCESS);
                break;
            default:
                // undefined state
                exit(EXIT_FAILURE);
                break;
        }
    }
}
