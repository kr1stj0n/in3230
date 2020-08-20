#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "chat.h"

int main (int argc, char *argv[])
{
	int opt, sflag = 0, cflag = 0;

	while ((opt = getopt(argc, argv, "sc")) != -1) {
        	switch (opt) {
        		case 's':
            			sflag = 1;
            			break;
			case 'c':
				cflag = 1;
				break;
        		default: /* '?' */
            			printf("Usage: %s"
				       "[-s] server mode"
				       "[-c] client mode\n", argv[0]);
            			exit(EXIT_FAILURE);
        	}
    	}

	if (sflag)
		server();
	else if (cflag)
		client();
	else
		printf("usage\n");

	return 0;
}
