/* connection file, to handle ftp stuff with client */

#ifndef CONNECT_H
#define CONNECT_H

#include "utils.h"

struct DataConnParams{ /* struct to pass in parameters */
    int conn_fd; /* file descriptor for socket */
    enum DataConnMode* requested_mode; /* the mode requested by user, should only take values PASV or PORT */
    struct DataConnFd* data_fd; /* file descriptors for the different sockets */
    char client_address[MAXLEN]; /* for PORT */
    char client_port[MAXLEN]; /* for PORT */
};

void * handle_client(void* p_connect_fd); /* takes in the pointer to fd index */
void * handle_data_transfer(void* p_params); /* takes in pointer to DataConnParams */
int handle_PASV_mode(int connect_fd, struct DataConnFd* p_data_fd);
int handle_PORT_mode(int connect_fd, struct DataConnFd* p_data_fd, char* client_address, char* client_port);
enum ClientState process_login(struct ClientRequest request, int connect_fd, char* username, char* password); /* login state */
enum ClientState process_select_mode(struct ClientRequest request, int connect_fd, enum DataConnMode* p_mode, struct DataConnFd* p_dataconn_fd); /* select mode state */
enum ClientState process_idle_mode(struct ClientRequest request, int connect_fd, enum DataConnMode* p_mode, struct DataConnFd* p_dataconn_fd); /* idle state */
#endif