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
#define PORT 8080
#define MAX_USER_QUEUE 1024
#define MAXLEN 128 /* maximum length for hostname and port string (for getnameinfo) */
#define SA struct sockaddr

void check(int ret_val, char* error_msg); /* unclutter code, check if return value < 0 (usually -1) */
int parse_args(char** argv); /* turn passed in arguments into port and working directory */
void handle_client(int connect_fd); /* argument is the fd index */