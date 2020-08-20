/*
 * A multiclient - server chat application using UNIX sockets
 * chat.c source file
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include "chat.h"

int debug = 0;

void server(void)
{
	struct sockaddr_un addr;
	int    sd, accept_sd;
	pid_t  child_pid;

	printf("\n*** IN3230 Multiclient chat server is running! ***\n"
	       "* Waiting for users to connect...\n");

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
		/* Accept incoming connection */
		if ((accept_sd = accept(sd, NULL, NULL)) == -1) {
			perror("accept()");
			continue;
		}
		/* Create a new child process to handle the connection,
		 * while the parent listens for new ones */
		child_pid = fork();

		if (child_pid < 0) {
			perror("fork()");
			exit(-1);
		} else if (child_pid == 0) {     /* child process that will handle the new incomming connection */
			close(sd);             /* the child closes the copy of the listening socket
			                          that inheritated from the parent */

			char *username = (char *)malloc(sizeof(char) * 8);

			if (read(accept_sd, username, sizeof(username)) <= 0) {
				perror("read");
				close(accept_sd);
				exit(EXIT_FAILURE);
			} else
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
	int    sd, rc;
	char   buf[256];
	char   username[8];

	printf("\n*** WELCOME TO IN3230 CHAT ***\n"
	       "* Please, Be Kind & Polite! *\n");
	sleep(2);
	printf("\nEnter your username: ");
	fgets(username, 10, stdin);

	sd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sd < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, "server", sizeof(addr.sun_path) - 1);

	rc = connect(sd, (struct sockaddr *)&addr, sizeof(addr));
	if ( rc < 0) {
    		perror("connect");
		close(sd);
    		exit(EXIT_FAILURE);
	}

	rc = write(sd, username, strlen(username) - 1);
	if (rc <= 0) {
		perror("write");
		close(sd);
		exit(EXIT_FAILURE);
	}

	do {
		memset(buf, 0, sizeof(buf));
		fgets(buf, sizeof(buf), stdin);
		if (write(sd, buf, strlen(buf) + 1) < 0) {
			perror("write()");
			close(sd);
			exit(EXIT_FAILURE);
		}
	} while(1);

	return;
}

void handle_client(int sd, char *username)
{
	char buf[256];
	int rc;

	while (1) {
		memset(buf, 0, sizeof(buf));
		rc = read(sd, buf, sizeof(buf));
		if (rc <= 0) {
			perror("read");
			close(sd);
			printf("%s left the chat...\n", username);
			free(username);   /* Free the allocated memory to avoid memory leaks */
			exit(-1);
		}
		
		printf("<%s>: %s\n", username, buf);
	}
}
