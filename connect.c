#include "connect.h"

/*
 * To handle each ftp client
 * @param p_connect_fd int pointer to connected socket file descriptor pointer
 */
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

    // for login state
    char current_username[MAXLEN] = {0};
    char current_password[MAXLEN] = {0};

    // for selectmode state
    enum DataConnMode* p_current_mode = malloc(sizeof(enum DataConnMode));
    *p_current_mode = NOTSET;
    pthread_mutex_t* p_current_mode_lock = malloc(sizeof(pthread_mutex_t));
    pthread_mutexattr_t* p_current_lock_attr = malloc(sizeof(pthread_mutexattr_t));

    // initialize mutex lock
    pthread_mutexattr_init(p_current_lock_attr);
    pthread_mutexattr_settype(p_current_lock_attr, PTHREAD_MUTEX_ERRORCHECK); /* set to safe mode */
    pthread_mutex_init(p_current_mode_lock, p_current_lock_attr); /* set attributes */

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
                current_state = process_select_mode(request_command, connect_fd, p_current_mode, p_current_mode_lock);
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
    free(p_current_mode);
    free(p_current_lock_attr);
    free(p_current_mode_lock);
    return NULL;
}

/*
 * Handles login
 * @param request the command that user typed in
 * @param connect_fd the file descriptor of the socket
 * @param p_username/p_password the username and password to be set
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
 * @param request the command that user typed in
 * @param connect_fd the file descriptor of the socket
 * @param p_data_transfer_mode points to the DataConnMode shared by handle_client
 * @param p_transfer_mode_lock mutex lock shared by handle_client
 * @returns Next state
 */
enum ClientState process_select_mode(struct ClientRequest request, int connect_fd, enum DataConnMode* p_current_mode,
        pthread_mutex_t* p_current_mode_lock){
    
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
        struct AddressPort client_ap = parse_address_port(request.parameter);
        if (isEmpty(client_ap.address)){
            char* resp_msg = "501 PORT parameter in incorrect format.\015\012";
            write(connect_fd, resp_msg, strlen(resp_msg));
        }else{
            // create struct to pass in parameters
            struct DataConnParams params;
            params.conn_fd = connect_fd;
            params.current_mode = p_current_mode;
            params.lock = p_current_mode_lock;
            params.requested_mode = PORT;
            strcpy(params.client_address, client_ap.address);
            strcpy(params.client_port, client_ap.port);

            pthread_t conn_thread;
            pthread_create(&conn_thread, NULL, handle_data_transfer, &params);
        }
    }else if (isEqual(request.verb, "PASV")){
        if (!isEmpty(request.parameter)){
            // PASV mode should have empty parameter
            char* resp_msg = "501 PASV should have empty parameter\015\012";
            write(connect_fd, resp_msg, strlen(resp_msg));
        }else{
            // create struct to pass in parameters
            struct DataConnParams params;
            params.conn_fd = connect_fd;
            params.current_mode = p_current_mode;
            params.lock = p_current_mode_lock;
            params.requested_mode = PASV;

            pthread_t conn_thread;
            pthread_create(&conn_thread, NULL, handle_data_transfer, &params);
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

/*
 * Process commands in SelectMode state
 * @param p_params is of type DataConnParams*
 * @returns Nothing
 */
void * handle_data_transfer(void* p_params){
    // get the input parameters out
    struct DataConnParams* p_data_params = p_params;
    int connect_fd = p_data_params->conn_fd;
    enum DataConnMode* p_current_mode = p_data_params->current_mode;
    pthread_mutex_t* p_current_mode_lock = p_data_params->lock;
    char* client_address = p_data_params->client_address;
    char* client_port = p_data_params->client_port;

    enum DataConnMode user_request_mode = p_data_params->requested_mode;

    if (user_request_mode == PASV){
        handle_PASV_mode(connect_fd, p_current_mode, p_current_mode_lock);
    }else if (user_request_mode == PORT){
        handle_PORT_mode(connect_fd, p_current_mode, p_current_mode_lock, client_address, client_port);
    }else{
        printf("Data Transfer: Unknown Mode, ask someone why this happened");
    }

    return NULL;
}

/*
 * Handles PASV mode
 * @returns 0 if fine, -1 if error
 */
int handle_PASV_mode(int connect_fd, enum DataConnMode* p_current_mode, pthread_mutex_t* p_transfer_mode_lock){

    struct sockaddr client_address; /* interface for the connection with client */
    socklen_t client_length = sizeof(client_address);

    struct AddressPort client_ap;

    getpeername(connect_fd, &client_address, &client_length);
    getnameinfo(&client_address, client_length, client_ap.address, MAXLEN,
        client_ap.port, MAXLEN, NI_NUMERICHOST|NI_NUMERICSERV); /* get info from socket, force numerical values */
    
    char ftp_address[BUFF_SIZE]; /* host port string in the format of ftp */
    bzero(ftp_address, BUFF_SIZE);

    to_ftp_address_port(client_ap, ftp_address);


    printf("%s ftp address = %s\n", client_ap.port, ftp_address);

    printf("In PASV mode\n");
    return 0;
}

/*
 * Handles PORT mode
 * @returns 0 if fine, -1 if error
 */
int handle_PORT_mode(int connect_fd, enum DataConnMode* p_current_mode, pthread_mutex_t* p_transfer_mode_lock, 
        char* client_address, char* client_port){
    
    printf("In PORT mode, trying to connect to %s with port %s\n", client_address, client_port);
    
    return 0;
}