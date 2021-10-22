/* connection file, to handle ftp stuff with client */

#ifndef CONNECT_H
#define CONNECT_H

#include "utils.h"

struct DataConnParams{ /* struct to pass in parameters */
    int conn_fd; /* file descriptor for socket */
    pthread_mutex_t* lock; /* mutex lock only used for DataConnMode */
    enum DataConnMode* current_mode; /* the conn_mode used to send data from main thread (on when to close, etc) */
    enum DataConnMode requested_mode; /* the mode requested by user, should only take values PASV or PORT */
    char client_address[MAXLEN]; /* for PORT */
    char client_port[MAXLEN]; /* for PORT */
};

void * handle_client(void* p_connect_fd); /* takes in the pointer to fd index */
void * handle_data_transfer(void* p_params); /* takes in pointer to DataConnParams */ 
int handle_PASV_mode(int connect_fd, enum DataConnMode* p_current_mode, pthread_mutex_t* p_transfer_mode_lock); 
int handle_PORT_mode(int connect_fd, enum DataConnMode* p_current_mode, pthread_mutex_t* p_transfer_mode_lock, char* client_address, char* client_port);
enum ClientState process_login(struct ClientRequest request, int connect_fd, char* username, char* password); /* login state */
enum ClientState process_select_mode(struct ClientRequest request, int connect_fd, enum DataConnMode* p_mode, pthread_mutex_t* p_transfer_mode_lock); /* select mode state */
#endif