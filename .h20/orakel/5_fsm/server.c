#include <stdio.h>		/* standard input/output library functions */
#include <stdlib.h>             /* standard library definitions (macros) */
#include <unistd.h>             /* standard symbolic constants and types */
#include <string.h>             /* string operations (strncpy, memset..) */
#include <time.h>               /* struct timespec and clock_gettime    */
#include <sys/socket.h>         /* sockets operations */
#include <sys/un.h>             /* definitions for UNIX domain sockets */
#include <sys/epoll.h>          /* epoll */

#include "common.h"

int main()
{
    enum state s_state;
    struct timespec lastHelloSent;
    struct timespec lastHelloRecv;
    struct timespec timenow;
    struct epoll_event ev, events[MAX_EVENTS];
    char hello[6];
    int epollfd;

    int sock, asock, rc;

    sock = setup_server_sock();

    while (1) {
        /* Wait for incoming connection. */
        asock = accept(sock, NULL, NULL);
        if (asock == -1) {
            perror("accept");
            continue;
        }

        printf("accepted\n");
        /*
         * At this point, accept() was sucessful.
         * set s_state to INIT
         */

        s_state = INIT;
        epollfd = epoll_create1(0);
        if (epollfd == -1) {
            perror("epoll_create1");
            exit(EXIT_FAILURE);
        }

        ev.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
        ev.data.fd = asock;
        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, asock, &ev) == -1) {
            perror("epoll_ctl: asock");
            exit(EXIT_FAILURE);
        }

        while (1) {
            switch (s_state) {
                case INIT:
                    printf("sending hello\n");
                    rc = write(asock, "hello", 6);
                    clock_gettime(CLOCK_REALTIME, &lastHelloSent);
                    s_state = WAIT;
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
                            s_state = EXIT;
                            continue;
                        } else {
                            if (diff_time_ms(lastHelloSent, timenow) >= HELLO_INTERVAL) {
                                /* Time to send HELLO */
                                s_state = INIT;
                                continue;
                            }
                        }
                    } else {
                        /* epoll() was triggered -> read the HELLO */
                        if (read(asock, hello, 6 ) <= 0)
                            s_state = EXIT;
                        else {
                            printf("received hello\n");
                            clock_gettime(CLOCK_REALTIME, &lastHelloRecv);
                        }    
                    }
                    break;
                case EXIT:
                    close(asock);
                    exit(EXIT_SUCCESS);
                    break;
                default:
                    // undefined state
                    exit(EXIT_FAILURE);
                    break;
            }
        }
    }
}
