/*
 * File: chat.c
 */

#include <stdio.h>		/* standard input/output library functions */
#include <stdlib.h>             /* standard library definitions (macros) */
#include <unistd.h>             /* standard symbolic constants and types */
#include <string.h>             /* string operations (strncpy, memset..) */

#include <sys/socket.h>         /* sockets operations */
#include <sys/un.h>             /* definitions for UNIX domain sockets */

#include "chat.h"


void server(void)
{
	struct sockaddr_un addr;
	int    sd, accept_sd, rc;
	pid_t  child_pid;

	printf("\n*** IN3230 Multiclient chat server is running! ***\n"
	       "* Waiting for users to connect...\n");

	/* 1. Create local socket. */
	sd = socket(AF_UNIX, SOCK_STREAM, 0); 
	if (sd  == -1) {
		perror("socket()");
		exit(-1);
	}

	/*
         * For portability clear the whole structure, since some
         * implementations have additional (nonstandard) fields in
         * the structure.
         */

	memset(&addr, 0, sizeof(struct sockaddr_un));

	/* 2. Bind socket to socket name. */

	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, SOCKET_NAME, sizeof(addr.sun_path) - 1);

	/* Unlink the socket so that we can reuse the program.
	 * This is bad hack! Better solution with a lock file,
	 * or signal handling.
	 * Check https://gavv.github.io/articles/unix-socket-reuse
	 */
        unlink(SOCKET_NAME);
	
	rc = bind(sd, (const struct sockaddr *)&addr, sizeof(addr));
	if (rc == -1) {
		perror("bind");
		close(sd);
		exit(EXIT_FAILURE);
	}

	/*
         * 3. Prepare for accepting incomming connections.
	 * The backlog size is set to MAX_CONNS.
	 * So while one request is being processed other requests
	 * can be waiting.
         */

	rc = listen(sd, MAX_CONNS);
	if (rc == -1) {
    		perror("listen()");
		close(sd);
    		exit(EXIT_FAILURE);
	}

	while (1) {
		/* Wait for incoming connection. */
		accept_sd = accept(sd, NULL, NULL);
		if (accept_sd == -1) {
			perror("accept");
			continue;
		}

		/*
		 * At this point, accept() was sucessful.
		 * Create a new child process to handle the connection,
		 * while the parent listens for new ones.
		 */

		child_pid = fork();

		if (child_pid < 0) {
			perror("fork");
			exit(EXIT_FAILURE);
		} else if (child_pid == 0) {
			/* 
			 * the child process that will handle connection
			 * the child closes the copy of the listening socket
			 * that inheritated from the parent
			 */
			close(sd);

			/*
			 * Allocate memory for the username that the user will
			 * send to the server
			 */
			char *username = (char *)malloc(sizeof(char) * 8);
			if (username == NULL) {
				perror("malloc");
				close(accept_sd);
				exit(EXIT_FAILURE);
			}

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

	/* Unlink the socket. */
        unlink(SOCKET_NAME);

        exit(EXIT_SUCCESS);
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
	strncpy(addr.sun_path, SOCKET_NAME, sizeof(addr.sun_path) - 1);

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
		rc = write(sd, buf, strlen(buf));
		if (rc < 0) {
			perror("write");
			close(sd);
			exit(EXIT_FAILURE);
		}
	} while (1);
}

void handle_client(int sd, char *username)
{
	char buf[256];
	int rc;

	while (1) {
		memset(buf, 0, sizeof(buf));
		rc = read(sd, buf, sizeof(buf));
		if (rc <= 0) {
			close(sd);
			printf("%s left the chat...\n", username);
		 	/* Free the allocated memory to avoid memory leaks */
			free(username);
			/* Only the child process exits here... */
			exit(EXIT_FAILURE); 
		}
		
		printf("<%s>: %s\n", username, buf);
	}
}
