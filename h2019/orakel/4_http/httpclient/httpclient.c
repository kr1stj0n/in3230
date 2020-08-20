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

#include "httpclient.h"


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
 *  NON BLOCKING SOCKET
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
 * PARSE URL FUNCTION
 */
void parse_url(const char *url, char *host, int *port, char *file_name)
{
    int start = 0;
    *port     = 80;
    int j     = 0;
    char *patterns[] = {"http://", "https://", NULL};

    for (int i = 0; patterns[i]; i++)
        if (strncmp(url, patterns[i], strlen(patterns[i])) == 0)
            start = strlen(patterns[i]);

    for (int i = start; url[i] != '/' && url[i] != '\0'; i++, j++)
        host[j] = url[i];
    host[j] = '\0';

    char *pos = strstr(host, ":");
    if (pos)
        sscanf(pos, ":%d", port);

    for (int i = 0; i < (int)strlen(host); i++)
    {
        if (host[i] == ':')
        {
            host[i] = '\0';
            break;
        }
    }

    j = 0;
    for (int i = start; url[i] != '\0'; i++)
    {
        if (url[i] == '/')
        {
            if (i !=  strlen(url) - 1)
                j = 0;
            continue;
        }
        else
            file_name[j++] = url[i];
    }
    file_name[j] = '\0';
}


/*
 *   NREAD FUNCTION
 */
int Nread(int fd, char *buf, size_t count)
{
	size_t  nleft = count;
	int     ret   = 0;

	while (nleft > 0) {
		ret = poll_fd(fd, 'r', 3000);
		if (ret <= 0) {
            fprintf(stderr, "\nERROR during poll()\n");
			return NET_SOFTERROR;
		}

		ret = read(fd, buf, nleft);
		if (ret > 0) {
            nleft -= ret;
            buf   += ret;
            continue;
        }
        else if (ret < 0) {
                switch(errno) {
                    case EINTR:
                    case EAGAIN:
#if EWOULDBLOCK != EAGAIN
                    case EWOULDBLOCK:
#endif
                        continue;

                    default:
                        return NET_HARDERROR;
                }
        }
		else // ret = 0
			break;
    }
	return count - nleft;
}


/*
 * PARSE RESPONSE
 */
struct HTTP_RES_HEADER parse_header(const char *response)
{
    struct HTTP_RES_HEADER resp;

    char *pos = strstr(response, "HTTP/");
    if (pos)
        sscanf(pos, "%*s %d", &resp.status_code);

    pos = strstr(response, "Content-Type:");
    if (pos)
        sscanf(pos, "%*s %s", resp.content_type);

    pos = strstr(response, "Content-length:");
    if (pos)
        sscanf(pos, "%*s %ld", &resp.content_length);

    return resp;
}


/*
 *  GET HOST BY NAME
 */
void get_ip_addr(char *host_name, char *ip_addr)
{
    struct hostent *host = gethostbyname(host_name);
    if (!host)
    {
        ip_addr = NULL;
        return;
    }

    for (int i = 0; host->h_addr_list[i]; i++)
    {
        strcpy(ip_addr, inet_ntoa( * (struct in_addr*) host->h_addr_list[i]));
        break;
    }
}


/*
 *  GET FILE SIZE
 */
unsigned long get_file_size(const char *filename)
{
    struct stat buf;

    if (stat(filename, &buf) < 0)
        return 0;

    return (unsigned long)buf.st_size;
}


/*
 *  DOWNLOAD FUNCTION
 */
void download(int sock, char *file_name, unsigned long content_length)
{
    unsigned long hasrecieve = 0;
    size_t        buf_size   = 65536;
    char          buf[65536] = {0};
    int           len;

	int fp = open(file_name, O_CREAT | O_WRONLY, S_IRWXG | S_IRWXO | S_IRWXU);
	if (fp < 0) {
		perror("fopen");
		exit(EXIT_FAILURE);
	}

	while (hasrecieve < content_length) {
		len = Nread(sock, buf, buf_size);
		if(len > 0) {
			write(fp, buf, len);
			hasrecieve += len;
		}
		else
            break;
	}

	close(fp);

    printf("Downloaded  %lu / %lu bytes.\n", hasrecieve, content_length);

    if (hasrecieve == content_length)
        printf("It is finished! :-)\n");
    else {
        printf("Download failed! :-(\n");
        remove(file_name);
    }

    return;
}
