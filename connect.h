/* to handle connections with client */
#ifndef CONNECT_H
#define CONNECT_H

#include "utils.h"

void * handle_client(void* p_connect_fd); /* argument is the pointer to fd index */
#endif