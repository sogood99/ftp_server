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

// data connection mode in (PASV or PORT)
enum DataConnMode{
    NOTSET, /* idle mode, next state (if exits) is CREATING_SOCEKT or PORT */
    CREATING_SOCKET, /* trying to create a socket with listen (PASV) */
    PASV, /* established a listening port, not yet connected */
    PASV_CONNECTED, /* connected to something */
    PORT, /* recorded the client's address */
    PORT_CONNECTING, /* attempting to establish connection */
    PORT_CONNECTED, /* connected to client's server */
    CLOSING, /* closing sockets, next state is NOTSET */
};

// client request
struct ClientRequest{
    char verb[MAXLEN];
    char parameter[MAXLEN]; /* optional */
};

// struct for address port in PORT method
struct AddressPort{
    char address[MAXLEN];
    char port[MAXLEN];
};

int read_buffer(int fd, char* buffer, size_t buffer_size, char* outer_buffer, char** p_last_read);
int create_listen_socket(char* host, char* port);
int create_connect_socket(char* hostname, char* port);
void parse_args(char** argv);
void to_ftp_address_port(struct AddressPort ap, char* ftp_output);
int isPrefix(char* string, char* prefix);
int isSuffix(char* string, char* suffix);
int isAlphabet(char c);
int isNumeric(char c);
int isEmpty(char* string);
int isEqual(char* string_a, char* string_b);
enum DataConnMode get_connection_mode(pthread_mutex_t* p_lock, enum DataConnMode* p_value);
void set_connection_mode(pthread_mutex_t* p_lock, enum DataConnMode* p_value, enum DataConnMode new_value);
struct ClientRequest parse_request(char* buffer);
struct AddressPort parse_address_port(char* buffer);
#endif