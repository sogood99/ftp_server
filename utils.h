#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <pthread.h>
#include <unistd.h>

#define BUFF_SIZE 1024
#define DEFAULT_PORT 8080
#define MAX_USER_QUEUE 1024
#define MAXLEN 256 /* maximum length for strings like hostname, port, path etc */
#define SA struct sockaddr

struct ServerParams{
    char root_directory[MAXLEN];
    int port;
}g_current_server_params;

// user state for each thread (client connection)
enum ClientState{
    Login, /* Waiting for username and password */
    SelectMode, /* Select PORT or PASV */
    Idle, /* logged in */
    Transfer, /* file is transfering */
    Exit, /* Exiting */
};

// client request
struct ClientRequest{
    char verb[MAXLEN];
    char parameter[MAXLEN]; /* optional */
};

void check_error(int ret_val, char* error_msg); /* unclutter code, check if return value < 0 (usually error = -1) */
void parse_args(char** argv); /* turn passed in arguments into port and working directory */
int isPrefix(char* string, char* prefix); /* check if prefix, true = 1, false = 0 */
int isSuffix(char* string, char* suffix); /* check if suffix, true = 1, false = 0 */
int isAlphabet(char c); /* check if char is ascii alphabet, true = 1, false = 0 */
struct ClientRequest parse_request(char* buffer); /* parse client request from buffer */
#endif