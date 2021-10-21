/* connection file, to handle ftp stuff with client */

#ifndef CONNECT_H
#define CONNECT_H

#include "utils.h"

void * handle_client(void* p_connect_fd); /* argument is the pointer to fd index */
enum ClientState process_login(struct ClientRequest request, int connect_fd, char* username, char* password); /* login state */
enum ClientState process_select_mode(struct ClientRequest request, int connect_fd, enum DataConnMode* mode); /* select mode state */
int create_pasv_socket(); /* create nessisary sockets for pasv mode, -1 if error, 0 if good */
int create_port_socket(); /* create nessisary sockets for pasv mode, -1 if error, 0 if good*/
#endif