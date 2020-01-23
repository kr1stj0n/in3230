#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#include "httpserver.h"


int main(int argc, char const *argv[])
{
	int sock, port, optval, csock, ret;
	struct sockaddr_in saddr;
	struct sockaddr_in caddr;
    socklen_t clen = sizeof(caddr);
	unsigned long file_size;
	char response[256] = {0};
	char filename[32]  = {0};
	char request[2048] = {0};

  	/* check command line args */
  	if (argc != 2) {
    	fprintf(stderr, "Usage: %s <port>\n", argv[0]);
    	exit(EXIT_FAILURE);
  	}
  	port = atoi(argv[1]);

  	/* open socket descriptor */
  	sock = socket(AF_INET, SOCK_STREAM, 0);
  	if (sock < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

  	/* allows us to restart server immediately */
  	optval = 1;
  	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

  	/* bind port to socket */
  	bzero((char *) &saddr, sizeof(saddr));
  	saddr.sin_family = AF_INET;
  	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
  	saddr.sin_port = htons((unsigned short)port);

  	if (bind(sock, (struct sockaddr *) &saddr, sizeof(saddr)) < 0) {
		perror("bind");
	  	close(sock);
	  	exit(EXIT_FAILURE);
  	}

  	/* get us ready to accept connection requests */
	if (listen(sock, 5) < 0) {
		perror("listen");
	  	close(sock);
	  	exit(EXIT_FAILURE);
  	}

  	/* wait for a connection request */
    csock = accept(sock, (struct sockaddr *) &caddr, &clen);
    if (csock < 0) {
		perror("accept");
		close(sock);
		exit(EXIT_FAILURE);
	}

	set_nonblocking(csock);

	ret = poll_fd(csock, 'r', 3000);
	if (ret <= 0) {
		fprintf(stderr, "ERROR during poll()\n");
        close(csock);
        close(sock);
	    exit(EXIT_FAILURE);
	}

    ret = read(csock, request, 2048);
    if (ret <= 0) {
        perror("read");
        close(csock);
        close(sock);
        exit(EXIT_FAILURE);
    }
    request[ret] = '\0';

	file_size = parse_get_request(request, filename);

	sprintf(response, \
            "HTTP/1.1 200 OK\n"\
            "Content-length: %ld\n"\
            "\r\n"\
            ,file_size);

	ret = Nwrite(csock, response, strlen(response));
	if (ret < 0) {
		perror("Nwrite");
		close(csock);
        close(sock);
		exit(EXIT_FAILURE);
	}

    ret = poll_fd(csock, 'r', 3000);
    if (ret <= 0) {
        fprintf(stderr, "ERROR during poll()\n");
        close(csock);
        close(sock);
        exit(EXIT_FAILURE);
    }

	upload(csock, filename, file_size);

    shutdown(csock, SHUT_RDWR);
    sleep(1);     /* FIXME: Close properly */
    close(csock);
    close(sock);

    return 0;
}
