#ifndef _HTTPCLIENT_H_
#define _HTTPCLIENT_H_

#include <stddef.h>

#define NET_SOFTERROR -1
#define NET_HARDERROR -2

struct HTTP_RES_HEADER
{
    int status_code;               //HTTP/1.1 '200' OK
    char content_type[128];        //Content-Type: application/gzip
    unsigned long content_length;  //Content-Length: 11683079
};


int           poll_fd(int, char, int);
int           Nread(int, char *, size_t);
void          parse_url(const char *, char *, int *, char *);
struct        HTTP_RES_HEADER parse_header(const char *);
void          get_ip_addr(char *, char *);
void          set_nonblocking(int);
unsigned long get_file_size(const char *);
void          download(int, char *, unsigned long);


#endif // #ifndef _HTTPCLIENT_H_
