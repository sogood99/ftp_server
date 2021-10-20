#include "connect.h"

void * handle_client(void* p_connect_fd){
    int connect_fd = *((int *) p_connect_fd);
    free(p_connect_fd);

    char buffer[BUFF_SIZE];
    size_t bytes_read;
    bzero(buffer, sizeof(buffer)); /* clear buffer */
    while ((bytes_read = read(connect_fd, buffer, sizeof(buffer)) > 0)){
        printf("%s\n", buffer);
        bzero(buffer, sizeof(buffer));
    }
    printf("Connection Closed\n");
    close(connect_fd);
    return NULL;
}