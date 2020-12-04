/*
 * File: chat.h
 *
 * Good practises for C header files.
 * 1. ALWAYS #include guards!
 * 2. #include only necessary libraries!
 * 3. DO NOT define global variables!
 * 4. #define MACROS and declare functions prototype to be shared between
 *    .c source files.
 * 
 * Check https://valecs.gitlab.io/resources/CHeaderFileGuidelines.pdf for some
 * more nice practises. 
 */

/*
 * #ifndef and #define are known as header guards.
 * Their primary purpose is to prevent header files
 * from being included multiple times.
 */

#ifndef _CHAT_H
#define _CHAT_H

#define SOCKET_NAME "server.socket"
#define MAX_CONNS 5 /* max. length of the pending connections queue */

/*
 * We declare the signature of a function in the header file
 * and its definition in the source file.
 *
 * return_type function_name(parameter1, parameter2, ...);
 */

void server(void);
void client(void);
void handle_client(int, char *);

#endif
