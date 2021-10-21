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
    char current_username[MAXLEN] = {0};
    char current_password[MAXLEN] = {0};

    char* hello_msg = "220 FTP server ready\012";
    char* unknown_format_msg = "500 Unknown Request Format\012";
    write(connect_fd, hello_msg, strlen(hello_msg));

    while ((bytes_read = read(connect_fd, buffer, sizeof(buffer)) > 0)){
        // keep on reading commands

        // check request format validity
        struct ClientRequest request_command = parse_request(buffer);
        if (isEmpty(request_command.verb)){
            // parse error
            write(connect_fd, unknown_format_msg, strlen(unknown_format_msg));
        }else{
            // run the command
            if (current_state == Login){
                current_state = process_login(request_command, connect_fd, current_username, current_password);
            }
        }
        // clear buffer
        bzero(buffer, BUFF_SIZE);
    }

    // ends connection
    printf("Connection Closed\n");
    close(connect_fd);
    return NULL;
}

enum ClientState process_login(struct ClientRequest request, int connect_fd, char* username, char* password){
    // Handles login, returns next state
    if (strcmp(request.verb, "USER") == 0){
        // USER command
        if (isEmpty(request.parameter)){
            char* resp_msg = "530 Username Unacceptable\012";
            write(connect_fd, resp_msg, strlen(resp_msg));
        }else if (strcmp(request.parameter, "anonymous") == 0){
            // anonymous login
            strcpy(username, request.parameter);
            char* resp_msg = "331 Guest Login OK, send email as password\012";
            write(connect_fd, resp_msg, strlen(resp_msg));
        }else{
            strcpy(username, request.parameter);
            char* resp_msg = "230 Username OK\012";
            write(connect_fd, resp_msg, strlen(resp_msg));
        }
        // clears previous password
        bzero(password, MAXLEN);
    }else if (strcmp(request.verb, "PASS") == 0){
        // PASS command
        // currently only supports guest
        if (isEmpty(username)){
            char* resp_msg = "503 Please enter username.\012";
            write(connect_fd, resp_msg, strlen(resp_msg));
        }else if (strcmp(username, "anonymous") == 0){
            // anonymous login
            if (!isEmpty(request.parameter)){
                // password not empty
                strcpy(password, request.parameter);
                char* resp_msg = "230 Guest login ok, access restrictions apply.\012";
                write(connect_fd, resp_msg, strlen(resp_msg));
                
                return SelectMode;
            }else{
                // password empty
                char* resp_msg = "530 Username Password Combination Not Recognized.\012";
                write(connect_fd, resp_msg, strlen(resp_msg));
            }
        }else{
            // currently only supports anonymous login
            // TODO, write non-anonymous login
            char* resp_msg = "530 Username Password Combination Not Recognized.\012";
            write(connect_fd, resp_msg, strlen(resp_msg));
        }
    }else if (strcmp(request.verb, "ACCT") == 0){
        char* resp_msg = "502-Currently Doesnt Support ACCT\012";
        write(connect_fd, resp_msg, strlen(resp_msg));
    }else{
        char* resp_msg = "530-Please Login\012";
        write(connect_fd, resp_msg, strlen(resp_msg));
    }
    return Login;
}