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
#include <sys/poll.h>
#include <errno.h>

#define READ read

struct HTTP_RES_HEADER
{
    int status_code;//HTTP/1.1 '200' OK
    char content_type[128];//Content-Type: application/gzip
    unsigned long content_length;//Content-Length: 11683079
};

/*
 * SELECT FUNCTION
 */
int poll_fd (int fd, int maxtime, int writep)
{
	int    res;
	struct pollfd pfd;

	pfd.fd = fd;
	pfd.events = POLLIN;

	do {

		res = poll (&pfd, (nfds_t)1, -1);

	} while(res == -1 && errno == EINTR);

	return res;
}
/*
 * READ FUNCTION
 */
int iread (int fd, char *buf, int len)
{
  int res;

	do {
		res = poll_fd (fd, 1, -1);
		if (res <= 0) {
			/* Set errno to ETIMEDOUT on timeout.  */
			if (res == 0)
				errno = ETIMEDOUT;
	      return -1;
	    }
      res = READ (fd, buf, len);

	  if(res == 0)
		  break;
	}
  while (res == -1 && errno == EINTR);

  return res;
}

/*
 * PARSE URL FUNCTION
 */
void parse_url(const char *url, char *host, int *port, char *file_name)
{
    int j = 0;
    int start = 0;
    *port = 80;
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
 * GET HOST BY NAME
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
 * NON BLOCKING SOCKET
 */
void set_nonblocking(int fd)
{
    int ret = fcntl(fd, F_SETFL, O_NONBLOCK);
    if (ret) {
        perror("fcntl(F_SETFL, O_NONBLOCK");
    }
    return;
}

/*
 * GET FILE SIZE
 */
unsigned long get_file_size(const char *filename)
{
    struct stat buf;
    if (stat(filename, &buf) < 0)
        return 0;
    return (unsigned long) buf.st_size;
}

/*
 * DOWNLOAD FUNCTION
 */
void download(int client_socket, char *file_name, unsigned long content_length)
{
    unsigned long hasrecieve = 0;
    int mem_size = 65536;
    int buf_len = mem_size;
    int len;

    int fd = open(file_name, O_CREAT | O_WRONLY, S_IRWXG | S_IRWXO | S_IRWXU);
    if (fd < 0)
    {
        perror("open");
        return;
    }

    char *buf = (char *) malloc(mem_size);

    while (hasrecieve < content_length) {
		len = iread(client_socket, buf, buf_len);
		if(len > 0) {
			write(fd, buf, len);
			hasrecieve += len;
		}
		if(len == 0)
			break;
        if (hasrecieve == content_length)
            break;
    }

    free(buf);

    return;
}

int main(int argc, char const *argv[])
{
    char url[256]      = "127.0.0.1";
    char host[64]      = {0};
    char ip_addr[16]   = {0};
    int  port          = 80;
    char file_name[32] = {0};
    char header[2048]  = {0};
    char response[256] = {0};
    int  client_socket, res, len;

    if (argc == 1) {
		perror("argc");
		exit(0);
    }
    else
 		strcpy(url, argv[1]);

    parse_url(url, host, &port, file_name);

    get_ip_addr(host, ip_addr);
    if (strlen(ip_addr) == 0) {
		printf("Bad IP address\n");
		return 0;
    }


    sprintf(header, \
            "GET %s HTTP/1.1\r\n"\
            "Accept: */*\r\n"\
            "User-Agent: My Wget Client\r\n"\
            "Host: %s\r\n"\
            "Connection: Keep-Alive\r\n"\
            "\r\n"\
            ,file_name, host);

    client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_socket < 0) {
		perror("socket");
		exit(-1);
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip_addr);
    addr.sin_port = htons(port);

    res = connect(client_socket, (struct sockaddr *) &addr, sizeof(addr));
    if (res == -1) {
		perror("connect");
		exit(-1);
    }

    set_nonblocking(client_socket);

    write(client_socket, header, strlen(header));

	res = poll_fd(client_socket, 2, 0);
	if (res <= 0) {
		/* Set errno to ETIMEDOUT on timeout.  */
		if (res == 0)
			errno = ETIMEDOUT;
	    return -1;
	}
    len = read(client_socket, response, 64);
    response[len] = '\0';

    struct HTTP_RES_HEADER resp = parse_header(response);

    if (resp.status_code != 200)
    {
        printf("File cannot be downloaded: %d\n", resp.status_code);
        goto quit;
    }

    download(client_socket, file_name, resp.content_length);

    if (resp.content_length == get_file_size(file_name))
        printf("Done! :-)\n");
    else
    {
        remove(file_name);
        printf("\nDownload failed :(\n\n");
    }

 quit:
    shutdown(client_socket, 2);
    return 0;
}
