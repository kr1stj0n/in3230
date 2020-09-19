#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#include "httpclient.h"


int main(int argc, char const *argv[])
{
    char url[64]      = "127.0.0.1";
    char host[32]      = {0};
    char ip_addr[16]   = {0};
    char file_name[32] = {0};
    char header[2048]  = {0};
    char response[256] = {0};
    int  port          = 80;
    int  sock, ret;

    if (argc == 1) {
        printf("Usage: ./httpclient http://url:port/file\n");
		exit(EXIT_FAILURE);
    }
    else
 		strcpy(url, argv[1]);

    parse_url(url, host, &port, file_name);

    get_ip_addr(host, ip_addr);
    if (strlen(ip_addr) == 0) {
		fprintf(stderr, "Bad IP address!\n");
		exit(EXIT_FAILURE);
    }

    sprintf(header, \
            "GET %s HTTP/1.1\r\n"\
            "Accept: */*\r\n"\
            "User-Agent: My Wget Client\r\n"\
            "Host: %s\r\n"\
            "Connection: Keep-Alive\r\n"\
            "\r\n"\
            ,file_name, host);

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip_addr);
    addr.sin_port = htons(port);

    ret = connect(sock, (struct sockaddr *) &addr, sizeof(addr));
    if (ret < 0) {
		perror("connect");
        close(sock);
		exit(EXIT_FAILURE);
    }

    set_nonblocking(sock);

    ret = write(sock, header, strlen(header));
    if (ret < 0) {
        perror("write");
        close(sock);
        exit(EXIT_FAILURE);
    }

	ret = poll_fd(sock, 'r', 3000);
	if (ret <= 0) {
        fprintf(stderr, "ERROR during poll()\n");
        close(sock);
	    exit(EXIT_FAILURE);
	}

    ret = read(sock, response, 256);
    if (ret <= 0) {
        perror("read");
        close(sock);
        exit(EXIT_FAILURE);
    }
    response[ret] = '\0';

    struct HTTP_RES_HEADER resp = parse_header(response);

    if (resp.status_code != 200)
    {
        printf("File cannot be downloaded: %d\n", resp.status_code);
        close(sock);
        exit(EXIT_FAILURE);
    }

    ret = write(sock, "OK", 3); /* FIXME: Check errors during write */
    if (ret < 0) {
        perror("write");
        close(sock);
        exit(EXIT_FAILURE);
    }

    download(sock, file_name, resp.content_length);

    shutdown(sock, SHUT_WR);
    close(sock);

    return 0;
}
