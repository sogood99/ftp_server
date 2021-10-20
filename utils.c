#include "utils.h"

void check(int ret_val, char* error_msg){
    /* Prints error if error value == -1 */
    if (ret_val < 0){
        printf("%s", error_msg);
        exit(EXIT_FAILURE);
    }
}

int parse_args(char** argv){
    // TODO: take in argc values and produce dir location and port value
    return 8080;
}

void handle_client(int connect_fd){
    char buffer[BUFF_SIZE];
    size_t bytes_read;
    bzero(buffer, sizeof(buffer)); /* clear buffer */
    while ((bytes_read = read(connect_fd, buffer, sizeof(buffer)) > 0)){
        printf("%s\n", buffer);
        bzero(buffer, sizeof(buffer));
    }
    printf("Connection Closed\n");
    close(connect_fd);
}