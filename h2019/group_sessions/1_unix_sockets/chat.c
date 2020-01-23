/*
 * A multiclient - server chat application using UNIX sockets
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#define MAX_CONNS 5 /* the maximum length for the queue of pending connections */

void client(void);
void server(void);
void handle_client(int, char *);


void server(void)
{
	struct sockaddr_un addr;
	int    sd, accept_sd;
	pid_t  child_pid;
	char   question[16] = "Who are you?\n";

	/* Create socket */
	if ((sd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		perror("socket()");
		exit(-1);
	}

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, "server", sizeof(addr.sun_path) - 1);

	unlink("server");
	
	/* Bind socket to the struct sockaddr_un addr */
	if (bind(sd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		perror("bind()");
		close(sd);
		exit(-1);
	}

	/* Put the socket in listening state... */
	if (listen(sd, MAX_CONNS) == -1) {
    		perror("listen()");
		close(sd);
    		exit(-1);
	}

	while (1) {
		if((accept_sd = accept(sd, NULL, NULL)) == -1) {   /* Accept incoming connection */
			perror("accept()");
			continue;
		}
		/* Create a new child process to handle the connection, meanwhile the parent listens for new ones */
		child_pid = fork();

		if (child_pid < 0) {
			perror("fork()");
			exit(-1);
		}
		else if (child_pid == 0) {     /* child process that will handle the new incomming connection */
			close(sd);             /* the child closes the copy of the listening socket
			                          that inheritated from the parent */

			if (write(accept_sd, question, strlen(question)) < 0) {
				perror("write()");
				close(accept_sd);
				exit(-1);
			}

			char *username = (char *)malloc(sizeof(char) * 8);

			if(read(accept_sd, username, sizeof(username)) <= 0) {
				perror("read()");
				close(accept_sd);
				exit(-1);
			}
			else
				printf("<INFO> %s joined the chat\n", username);

			handle_client(accept_sd, username);
		}
	}

	close(sd);

	return;
}


void client(void)
{
	struct sockaddr_un addr;
	int    sd, ret;
	char   buf[256];
	char   username[8];

	if ((sd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		perror("socket()");
		exit(-1);
	}

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, "server", sizeof(addr.sun_path) - 1);

	if (connect(sd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
    		perror("connect()");
		close(sd);
    		exit(-1);
	}

	if ((ret = read(sd, buf, sizeof(buf))) <= 0) {
		perror("read()");
		close(sd);
		exit(-1);
	}
	else {
		printf("%s\n", buf);

		fgets(username, 10, stdin);

		if (write(sd, username, strlen(username) - 1) < 0) {
			perror("write()");
			close(sd);
			exit(-1);
		}
	}

	do {
		memset(buf, 0, sizeof(buf));
		fgets(buf, sizeof(buf), stdin);
		if (write(sd, buf, strlen(buf) + 1) < 0) {
			perror("write()");
				close(sd);
				exit(-1);
			}

			if (strstr(buf, "ADIOS") != NULL) {
				close(sd);
				exit(-1);
			}
	} while(1);

	return;
}

void handle_client(int sd, char *username)
{
	char buf[256];

	do {
		memset(buf, 0, sizeof(buf));
		if (read(sd, buf, sizeof(buf)) <= 0) {
			perror("read()");
			close(sd);
			free(username);   /* Freeing the allocated memory to avoid memory leaks */
			exit(-1);
		}
		else {
			if (strstr(buf, "ADIOS") != NULL) {
				printf("%s left the chat...\n", username);
				close(sd);
				free(username);
				exit(-1);
			}
			printf("<%s>: %s\n", username, buf);
		}
	} while(1);

	return;
}

int main (int argc, char *argv[])
{
	int opt;

	printf("\n*** WELCOME TO IN3230 CHAT ***\n");

	while ((opt = getopt(argc, argv, "sc")) != -1) {
        	switch (opt) {
        		case 's':
            			server();
            			break;
			case 'c':
				client();
				break;
        		default: /* '?' */
            			printf("Usage: %s [-s] server mode [-c] client mode\n", argv[0]);
            			exit(EXIT_FAILURE);
        	}
    	}

	return 0;
}
