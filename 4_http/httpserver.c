#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/select.h>
#include <errno.h>

#define READ read

/*
 * SELECT FUNCTION
 */
int select_fd (int fd, int maxtime, int writep)
{
	fd_set read_fds, write_fds;
	struct timeval timeout;
	int    res;

	timeout.tv_sec = 0;
	timeout.tv_usec = maxtime;
	/* HPUX reportedly warns here.  What is the correct incantation?  */
	do {
		FD_ZERO (&read_fds);
		FD_SET (fd, &read_fds);
		FD_ZERO (&write_fds);
		FD_SET (fd, &write_fds);

		res = select (fd + 1, writep ? NULL : &read_fds, writep ? &write_fds : NULL, NULL, &timeout);
	} while(res == -1 && errno == EINTR);

	return res;
}


void set_nonblocking(int fd)
{
    int ret = fcntl(fd, F_SETFL, O_NONBLOCK);
    if (ret) {
        perror("fcntl(F_SETFL, O_NONBLOCK");
    }
    return;
}

unsigned long parse_get_request(char *request, char *filename)
{

    struct stat buf;

    char *pos = strstr(request, "GET");
    if (pos)
        sscanf(pos, "%*s %s", filename);

    if (stat(filename, &buf) < 0)
        return 0;
    return (unsigned long) buf.st_size;
}


int Nwrite(int fd, char *buf, size_t count)
{
    register ssize_t r;
    register size_t nleft = count;

    while (nleft > 0) {
	r = write(fd, buf, nleft);
	if (r < 0) {
	    switch (errno) {
		case EINTR:
		case EAGAIN:
#if (EAGAIN != EWOULDBLOCK)
		case EWOULDBLOCK:
#endif
		continue;

		case ENOBUFS:
		return -1;

		default:
		return -2;
	    }
	} else if (r == 0)
	    return -1;
	nleft -= r;
	buf += r;
    }
    return count;
}


void upload(int client_sock, char *file_name, unsigned long content_length)
{
    int mem_size = 65536;
    int len, res;

    FILE *fp = fopen(file_name, "rb");
    if (fp == NULL) {
        perror("open");
        return;
    }

    char *buf = (char *) malloc(mem_size);

	while( (len = fread(buf, 1, mem_size, fp)) > 0 ) {
	do {
		select_fd(client_sock, 100000, 1);
        	res = Nwrite(client_sock, buf, len);
		memset(buf, 0, mem_size);
        } while(res == -1 && errno == EAGAIN);
    }

    free(buf);

	printf("Done! :-)\n");

    return;
}

int main(int argc, char const *argv[])
{
	int sock, port, optval, client_sock, res, len;
	socklen_t clientlen;
	struct sockaddr_in serveraddr;
	struct sockaddr_in clientaddr;
	unsigned long file_size;
	char response[256];
	char filename[32] = {0};

	char request[2048]  = {0};

  	/* check command line args */
  	if (argc != 2) {
    	fprintf(stderr, "usage: %s <port>\n", argv[0]);
    	exit(1);
  	}
  	port = atoi(argv[1]);

  	/* open socket descriptor */
  	sock = socket(AF_INET, SOCK_STREAM, 0);
  	if (sock < 0) {
		perror("socket");
		exit(-1);
	}

  	/* allows us to restart server immediately */
  	optval = 1;
  	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

  	/* bind port to socket */
  	bzero((char *) &serveraddr, sizeof(serveraddr));
  	serveraddr.sin_family = AF_INET;
  	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  	serveraddr.sin_port = htons((unsigned short)port);

  	if (bind(sock, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) {
		perror("bind");
	  	close(sock);
	  	exit(-1);
  	}

  	/* get us ready to accept connection requests */
	if (listen(sock, 5) < 0) {
		perror("listen");
	  	close(sock);
	  	exit(-1);
  	}

  	clientlen = sizeof(clientaddr);
  	/* wait for a connection request */
    client_sock = accept(sock, (struct sockaddr *) &clientaddr, &clientlen);
    if (client_sock < 0) {
		perror("accept");
		close(sock);
		exit(-1);
	}

	set_nonblocking(client_sock);

	res = select_fd(client_sock, 1000000, 0);
	if (res <= 0) {
		/* Set errno to ETIMEDOUT on timeout.  */
		if (res == 0)
			errno = ETIMEDOUT;
	    return -1;
	}
    len = read(client_sock, request, 2048);
    request[len] = '\0';

	file_size = parse_get_request(request, filename);

	sprintf(response, \
            "HTTP/1.1 200 OK\n"\
            "Content-length: %ld\n"\
            "\r\n"\
            ,file_size);

	write(client_sock, response, strlen(response));

	upload(client_sock, filename, file_size);

    return 0;
}
