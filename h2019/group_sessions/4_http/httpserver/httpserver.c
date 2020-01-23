#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>

#include <arpa/inet.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "httpserver.h"


/*
 *   POLL FILE DESCRIPTOR
 */
int poll_fd(int fd, char mode, int timeout)
{
	struct pollfd pfd;
	int    ret;

	pfd.fd = fd;

    switch(mode) {
        case 'r':
            {
                pfd.events = POLLIN;
                do {
                    ret = poll(&pfd, (nfds_t)1, timeout);
                    if (ret > 0) {
                        if (pfd.revents != POLLIN)
                            return NET_HARDERROR;

                        return ret;
                    }
		            if (ret < 0) {
			            if (errno == EINTR)
				            continue;

                        perror("poll");
			            return ret;
		            }
		            if (ret == 0) {
			            errno = ETIMEDOUT;
			            perror("poll");
			            return ret;
		            }
                } while(ret < 0 && errno == EINTR);

                break;
            }
        case 'w':
            {
                pfd.events = POLLOUT;
                do {
                    ret = poll(&pfd, (nfds_t)1, timeout);
                    if (ret > 0) {
                        if (pfd.revents != POLLOUT)
                            return NET_HARDERROR;

                        return ret;
                    }
		            if (ret < 0) {
			            if (errno == EINTR)
				            continue;

                        perror("poll");
			            return ret;
		            }
		            if (ret == 0) {
			            errno = ETIMEDOUT;
			            perror("poll");
			            return ret;
		            }
                } while(ret < 0 && errno == EINTR);

                break;
            }
        default:
            {
                fprintf(stderr, "Undefined polling mode\n");
                exit (EXIT_FAILURE);
            }
    }

	return NET_SOFTERROR;
}


/*
 *  SET SOCKET TO O_NONBLOCK
 */
void set_nonblocking(int fd)
{
    int ret = fcntl(fd, F_SETFL, O_NONBLOCK);
    if (ret) {
        perror("fcntl");
        close(fd);
        exit(EXIT_FAILURE);
    }

    return;
}


/*
 *  PARSE GET REQUEST
 */
unsigned long parse_get_request(char *request, char *filename)
{
    struct stat buf;

    char *pos = strstr(request, "GET");
    if(pos)
        sscanf(pos, "%*s %s", filename);

    if(stat(filename, &buf) < 0)
        return 0;

    return (unsigned long)buf.st_size;
}


/*
 *    NWRITE FUNCTION
 */
int Nwrite(int fd, char *buf, size_t count)
{
	size_t   nleft = count;
    int      ret   = 0;

    while (nleft > 0) {
		ret = poll_fd(fd, 'w', 3000);
		if (ret <= 0) {
			fprintf(stderr, "\nERROR during poll()\n");
            return NET_SOFTERROR;
		}

		ret = write(fd, buf, nleft);
        if (ret > 0) {
            nleft -= ret;
            buf   += ret;
            continue;
        }
        else if (ret < 0) {
			switch (errno) {
				case EINTR:
				case EAGAIN:
#if EWOULDBLOCK != EAGAIN
                    case EWOULDBLOCK:
#endif
					continue;

				case ENOBUFS:
					return NET_SOFTERROR;

				default:
					return NET_HARDERROR;
			}
        }
         else // ret = 0
            return NET_SOFTERROR;
    }
	return count;
}


/*
 *    UPLOAD FILE
 */
void upload(int sock, char *file_name, unsigned long file_size)
{
    unsigned long hassent = 0;
	size_t  buf_size      = 65536;
	char    buf[65536]    = {0};
	int     len           = 0;
    int     ret           = 0;

    FILE *fp = fopen(file_name, "rb");
    if (fp == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

	while ((len = fread(buf, 1, buf_size, fp)) > 0 ) {
	    ret = Nwrite(sock, buf, len);
        if (ret < 0)
            break;

        hassent += ret;
	}

	fclose(fp);

	printf("Uploaded  %lu / %lu bytes.\n", hassent, file_size);

    if (hassent == file_size)
        printf("It is finished! :-)\n");
    else
        printf("Upload failed! :-(\n");

    return;
}
