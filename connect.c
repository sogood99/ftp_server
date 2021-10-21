#include "connect.h"

void * handle_client(void* p_connect_fd){
    // for pthreading
    int connect_fd = *((int *) p_connect_fd);
    free(p_connect_fd);

    // for socket
    char buffer[BUFF_SIZE];
    size_t bytes_read;
    bzero(buffer, sizeof(buffer)); /* clear buffer */

    /* -------- FTP Begins -------- */

    enum ClientState current_state = Login;
    char* current_username = "";
    char* current_password = "";

    char* hello_msg = "220-FTP server ready\012";
    char* unknown_format_msg = "500-Unknown Request Format\012";
    write(connect_fd, hello_msg, strlen(hello_msg));

    while ((bytes_read = read(connect_fd, buffer, sizeof(buffer)) > 0)){

        // check request format validity
        struct ClientRequest request_command = parse_request(buffer);

        if (current_state == Login){
            current_state = process_login(request_command, connect_fd, current_username, current_password);
        }
    }

    // ends connection
    printf("Connection Closed\n");
    close(connect_fd);
    return NULL;
}

enum ClientState process_login(struct ClientRequest request, int connect_fd, char* username, char* password){
    // Handles login, returns next state
    printf("processing login");
    return Login;
}