/* connection file, to handle ftp stuff with client */

#ifndef CONNECT_H
#define CONNECT_H

#include "utils.h"

struct DataConnParams{ /* struct to pass in parameters */
    pthread_mutex_t* lock; /* mutex lock only used for DataConnMode */
    enum DataConnMode* mode; /* the conn_mode used to send data from main thread (on when to close, etc) */
    enum DataConnMode requested_mode; /* the mode requested by user, should only take values PASV or PORT */
};

void * handle_client(void* p_connect_fd); /* takes in the pointer to fd index */
void * handle_data_transfer(void* p_params); /* takes in pointer to DataConnParams */ 
enum ClientState process_login(struct ClientRequest request, int connect_fd, char* p_username, char* p_password); /* login state */
enum ClientState process_select_mode(struct ClientRequest request, int connect_fd, enum DataConnMode* p_mode, pthread_mutex_t* p_transfer_mode_lock); /* select mode state */
#endif