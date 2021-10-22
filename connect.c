#include "connect.h"

void* handle_client(void* p_connect_fd){
    // for pthreading
    int connect_fd = *((int *) p_connect_fd);
    free(p_connect_fd);

    // for socket
    char buffer[BUFF_SIZE];
    size_t bytes_read;
    bzero(buffer, sizeof(buffer)); /* clear buffer */

    /* -------- FTP Code Begins -------- */

    enum ClientState current_state = Login;
    char current_username[MAXLEN] = {0};
    char current_password[MAXLEN] = {0};
    enum DataConnMode current_mode = NOTSET; /* in the future will be PASV or PORT */

    char* hello_msg = "220 FTP server ready\015\012";
    char* unknown_format_msg = "500 Unknown Request Format\015\012";
    write(connect_fd, hello_msg, strlen(hello_msg));

    while ((bytes_read = read(connect_fd, buffer, sizeof(buffer)) > 0)){ /* keep on reading commands */
        struct ClientRequest request_command = parse_request(buffer);
        if (isEmpty(request_command.verb)){ /* check command validity */
            // empty => parse error
            write(connect_fd, unknown_format_msg, strlen(unknown_format_msg));
        }else{
            // run the command, depends on current state
            switch (current_state){
            case Login:
                current_state = process_login(request_command, connect_fd, current_username, current_password);
                break;
            case SelectMode:
                current_state = process_select_mode(request_command, connect_fd, &current_mode);
                break;
            default:
                printf("Connector: Warning, switch statement should include all the states\n");
                break;
            }
            // if user quits
            if (current_state == Exit){
                break;
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

/*
 * Handles login
 * @returns Next state (ClientState)
 */
enum ClientState process_login(struct ClientRequest request, int connect_fd, char* username, char* password){
    if (isEqual(request.verb, "USER")){
        // USER command
        if (isEmpty(request.parameter)){
            char* resp_msg = "530 Username Unacceptable\015\012";
            write(connect_fd, resp_msg, strlen(resp_msg));
        }else if (isEqual(request.parameter, "anonymous")){
            // anonymous login
            strcpy(username, request.parameter);
            char* resp_msg = "331 Guest Login OK, send email as password\015\012";
            write(connect_fd, resp_msg, strlen(resp_msg));
        }else{
            strcpy(username, request.parameter);
            char* resp_msg = "331 Username OK\015\012";
            write(connect_fd, resp_msg, strlen(resp_msg));
        }
        // clears previous password
        bzero(password, MAXLEN);
    }else if (isEqual(request.verb, "PASS")){
        // PASS command
        // currently only supports guest
        if (isEmpty(username)){
            char* resp_msg = "501 Please enter username.\015\012";
            write(connect_fd, resp_msg, strlen(resp_msg));
        }else if (isEqual(username, "anonymous")){
            // anonymous login
            if (!isEmpty(request.parameter)){
                // password not empty
                strcpy(password, request.parameter);
                char* resp_msg = "230 Guest login OK, access restrictions may apply.\015\012";
                write(connect_fd, resp_msg, strlen(resp_msg));
                return SelectMode;
            }else{
                // password empty
                char* resp_msg = "530 Username Password Combination Not Recognized.\015\012";
                write(connect_fd, resp_msg, strlen(resp_msg));
            }
        }else{
            // currently only supports anonymous login
            // TODO, write non-anonymous login
            char* resp_msg = "530 Username Password Combination Not Recognized.\015\012";
            write(connect_fd, resp_msg, strlen(resp_msg));
        }
    }else if (isEqual(request.verb, "ACCT")){
        char* resp_msg = "502 Currently Doesnt Support ACCT\015\012";
        write(connect_fd, resp_msg, strlen(resp_msg));
    }else{
        char* resp_msg = "530 Please Login\015\012";
        write(connect_fd, resp_msg, strlen(resp_msg));
    }
    return Login;
}

/*
 * Process commands in SelectMode state
 * @return Next state
 */
enum ClientState process_select_mode(struct ClientRequest request, int connect_fd, enum DataConnMode* mode){
    if (isEqual(request.verb, "SYST")){
        // support SYST command
        char* resp_msg = "215 UNIX Type: L8\015\012";
        write(connect_fd, resp_msg, strlen(resp_msg));
    }else if (isEqual(request.verb, "TYPE")){
        // support TYPE command
        if (isEqual(request.parameter, "I")){
            char* resp_msg = "200 Type set to I.\015\012";
            write(connect_fd, resp_msg, strlen(resp_msg));
        }else{
            // currently doesnt support text or other format
            char* resp_msg = "504 Currently Only Supports Binary\015\012";
            write(connect_fd, resp_msg, strlen(resp_msg));
        }
    }else if (isEqual(request.verb, "PORT")){
        
    }else if (isEqual(request.verb, "PASV")){
        if (!isEmpty(request.parameter)){
            // PASV mode should have empty parameter
            char* resp_msg = "501 PASV should have empty parameter\015\012";
            write(connect_fd, resp_msg, strlen(resp_msg));
        }else{
            
        }
    }else if (isEqual(request.verb, "QUIT")){
        char* resp_msg = "221 Closing Connection, Goodbyte and goodbit.\015\012";
        write(connect_fd, resp_msg, strlen(resp_msg));
        return Exit;
    }else{
        char* resp_msg = "500 Unknown Command.\015\012";
        write(connect_fd, resp_msg, strlen(resp_msg));
    }
    return SelectMode;
}