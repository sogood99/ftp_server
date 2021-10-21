/* connection file, to handle ftp stuff with client */

#ifndef CONNECT_H
#define CONNECT_H

#include "utils.h"

void * handle_client(void* p_connect_fd); /* argument is the pointer to fd index */
enum ClientState process_login(struct ClientRequest request, int connect_fd, char* username, char* password); /* login state */
#endif