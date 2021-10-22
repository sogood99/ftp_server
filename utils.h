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
#define DEFAULT_IP "127.0.0.1"
#define DEFAULT_PORT "8080"
#define MAX_USER_QUEUE 1024
#define MAXLEN 256 /* maximum length for strings like hostname, port, path etc */
#define SA struct sockaddr

struct ServerParams{
    char root_directory[MAXLEN];
    char port[MAXLEN];
}g_current_server_params;

// user state for each thread (client connection)
enum ClientState{
    Login, /* Waiting for username and password */
    SelectMode, /* Select PORT or PASV */
    Connecting, /* Connecting to ftp client via PORT or PASV */
    Idle, /* finished connecting to PORT or PASV, awaiting instructions */
    Transfer, /* file is transfering */
    Exit, /* Exiting */
};

// data connection mode (PASV or PORT)
enum DataConnMode{
    NOTSET, /* idle mode, next state (if exists) is connect */
    CONNECTING, /* trying to establish a connection with listen (PASV) or connect (PORT) */
    PASV,
    PORT,
    CLOSING, /* closing sockets, next state is NOTSET */
};

// parameter to pass in for each PASV or PORT connection
struct DataConnParams{
    pthread_mutex_t* conn_mode_lock; /* mutex lock that is only used for locking conn_mode */
    enum DataConnMode* conn_mode; /* the conn_mode used to send data from main thread (when to close, etc) */
};

// client request
struct ClientRequest{
    char verb[MAXLEN];
    char parameter[MAXLEN]; /* optional */
};

int create_listen_socket(char* port);
int create_connect_socket(char* hostname, char* port);
void parse_args(char** argv);
int isPrefix(char* string, char* prefix);
int isSuffix(char* string, char* suffix);
int isAlphabet(char c);
int isEmpty(char* string);
int isEqual(char* string_a, char* string_b);
struct ClientRequest parse_request(char* buffer);
#endif