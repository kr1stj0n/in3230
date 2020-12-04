CC = gcc
CFLAGS  = -g -Wall
all:
	$(CC) $(CFLAGS) common.c client.c -o client
	$(CC) $(CFLAGS) common.c server.c -o server

clean:
	rm -f client server server_unix_path
