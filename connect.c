#include "connect.h"

/*
 * To handle each ftp client
 * @param p_connect_fd int pointer to connected socket file descriptor pointer
 */
void *handle_client(void *p_connect_fd)
{
    // for pthreading
    int connect_fd = *((int *)p_connect_fd);
    free(p_connect_fd);

    // for socket
    char buffer[BUFF_SIZE + 1], outer_buffer[BUFF_SIZE + 1], *last_read = outer_buffer;
    int did_read;                       /* bool if anything is left read from buffer, check out read_buffer function for more info */
    bzero(buffer, BUFF_SIZE + 1);       /* clear buffer */
    bzero(outer_buffer, BUFF_SIZE + 1); /* clear buffer */

    /* -------- FTP Code Begins -------- */

    enum ClientState current_state = Login;

    // for login state
    char current_username[MAXLEN] = {0};
    char current_password[MAXLEN] = {0};

    // for selectmode state
    enum DataConnMode *p_data_mode = malloc(sizeof(enum DataConnMode));
    *p_data_mode = NOTSET;
    struct DataConnFd *p_data_fd = malloc(sizeof(struct DataConnFd));
    init_dataconn_fd(p_data_fd);

    char *hello_msg = "220 FTP server ready\015\012";
    char *unknown_format_msg = "500 Unknown Request Format\015\012";
    write(connect_fd, hello_msg, strlen(hello_msg));

    while ((did_read = read_buffer(connect_fd, buffer, BUFF_SIZE, outer_buffer, &last_read)))
    { /* keep on reading commands */
        if (did_read == -1)
        { /* return value == -1 means read for another round, check doc for read_buffer */
            continue;
        }
        struct ClientRequest request_command = parse_request(buffer);
        if (isEmpty(request_command.verb))
        { /* check command validity */
            // empty => parse error
            write(connect_fd, unknown_format_msg, strlen(unknown_format_msg));
        }
        else if (isEqual(request_command.verb, "SYST"))
        { /* stateless commands */
            // support SYST command
            char *resp_msg = "215 UNIX Type: L8\015\012";
            write(connect_fd, resp_msg, strlen(resp_msg));
        }
        else
        {
            // run the command, depends on current state
            switch (current_state)
            {
            case Login:
                current_state = process_login(request_command, connect_fd, current_username, current_password);
                break;
            case SelectMode:
                current_state = process_select_mode(request_command, connect_fd, p_data_mode, p_data_fd);
                break;
            case Idle:
                current_state = process_idle_mode(request_command, connect_fd, p_data_mode, p_data_fd);
                break;
            default:
                printf("Connector: Warning, switch statement should include all the states\n");
                break;
            }
            // if user quits
            if (current_state == Exit)
            {
                break;
            }
        }
        // clear buffer
        bzero(buffer, BUFF_SIZE);
    }

    // ends connection
    printf("Connection Closed\n");
    close(connect_fd);
    free(p_data_mode);
    free(p_data_fd);
    return NULL;
}

/*
 * Handles login
 * @param request the command that user typed in
 * @param connect_fd the file descriptor of the socket
 * @param p_username/p_password the username and password to be set
 * @returns Next state (ClientState)
 */
enum ClientState process_login(struct ClientRequest request, int connect_fd, char *username, char *password)
{
    if (isEqual(request.verb, "USER"))
    {
        // USER command
        if (isEmpty(request.parameter))
        {
            char *resp_msg = "530 Username Unacceptable\015\012";
            write(connect_fd, resp_msg, strlen(resp_msg));
        }
        else if (isEqual(request.parameter, "anonymous"))
        {
            // anonymous login
            strcpy(username, request.parameter);
            char *resp_msg = "331 Guest Login OK, send email as password\015\012";
            write(connect_fd, resp_msg, strlen(resp_msg));
        }
        else
        {
            strcpy(username, request.parameter);
            char *resp_msg = "331 Username OK\015\012";
            write(connect_fd, resp_msg, strlen(resp_msg));
        }
        // clears previous password
        bzero(password, MAXLEN);
    }
    else if (isEqual(request.verb, "PASS"))
    {
        // PASS command
        // currently only supports guest
        if (isEmpty(username))
        {
            char *resp_msg = "501 Please enter username.\015\012";
            write(connect_fd, resp_msg, strlen(resp_msg));
        }
        else if (isEqual(username, "anonymous"))
        {
            // anonymous login
            if (!isEmpty(request.parameter))
            {
                // password not empty
                strcpy(password, request.parameter);
                char *resp_msg = "230 Guest login OK, access restrictions may apply.\015\012";
                write(connect_fd, resp_msg, strlen(resp_msg));
                return SelectMode;
            }
            else
            {
                // password empty
                char *resp_msg = "530 Username Password Combination Not Recognized.\015\012";
                write(connect_fd, resp_msg, strlen(resp_msg));
            }
        }
        else
        {
            // currently only supports anonymous login
            // TODO, write non-anonymous login
            char *resp_msg = "530 Username Password Combination Not Recognized.\015\012";
            write(connect_fd, resp_msg, strlen(resp_msg));
        }
    }
    else if (isEqual(request.verb, "ACCT"))
    {
        char *resp_msg = "502 Currently Doesnt Support ACCT\015\012";
        write(connect_fd, resp_msg, strlen(resp_msg));
    }
    else
    {
        char *resp_msg = "530 Please Login\015\012";
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
enum ClientState process_select_mode(struct ClientRequest request, int connect_fd, enum DataConnMode *p_current_mode,
                                     struct DataConnFd *p_data_fd)
{

    if (isEqual(request.verb, "TYPE"))
    {
        // support TYPE command
        if (isEqual(request.parameter, "I"))
        {
            char *resp_msg = "200 Type set to I.\015\012";
            write(connect_fd, resp_msg, strlen(resp_msg));
        }
        else
        {
            // currently doesnt support text or other format
            char *resp_msg = "504 Currently Only Supports Binary\015\012";
            write(connect_fd, resp_msg, strlen(resp_msg));
        }
    }
    else if (isEqual(request.verb, "PORT"))
    {
        struct AddressPort client_ap = parse_address_port(request.parameter); /* parse address */
        if (isEmpty(client_ap.address))
        {
            char *resp_msg = "501 PORT parameter in incorrect format.\015\012";
            write(connect_fd, resp_msg, strlen(resp_msg));
        }
        else
        {
            // create struct to pass in parameters
            *p_current_mode = PORT;

            struct DataConnParams params;
            params.conn_fd = connect_fd;
            params.data_fd = p_data_fd;
            params.requested_mode = p_current_mode;
            strcpy(params.client_address, client_ap.address);
            strcpy(params.client_port, client_ap.port);

            pthread_t conn_thread;
            pthread_create(&conn_thread, NULL, handle_data_transfer, &params);
            return Idle;
        }
    }
    else if (isEqual(request.verb, "PASV"))
    {
        if (!isEmpty(request.parameter))
        {
            // PASV mode should have empty parameter
            char *resp_msg = "501 PASV should have empty parameter\015\012";
            write(connect_fd, resp_msg, strlen(resp_msg));
        }
        else
        {
            // create struct to pass in parameters
            *p_current_mode = PASV;

            struct DataConnParams params;
            params.conn_fd = connect_fd;
            params.data_fd = p_data_fd;
            params.requested_mode = p_current_mode;

            pthread_t conn_thread;
            pthread_create(&conn_thread, NULL, handle_data_transfer, &params);
            return Idle;
        }
    }
    else if (isEqual(request.verb, "QUIT"))
    {
        char *resp_msg = "221 Closing Connection, Goodbyte and goodbit.\015\012";
        write(connect_fd, resp_msg, strlen(resp_msg));
        return Exit;
    }
    else
    {
        char *resp_msg = "500 Unknown Command.\015\012";
        write(connect_fd, resp_msg, strlen(resp_msg));
    }
    return SelectMode;
}

/*
 * Process commands in SelectMode state
 * @param p_params is of type DataConnParams*
 * @returns Nothing
 */
void *handle_data_transfer(void *p_params)
{
    // get the input parameters out
    struct DataConnParams *p_data_params = p_params;
    int connect_fd = p_data_params->conn_fd;
    struct DataConnFd *p_data_fd = p_data_params->data_fd;
    char *client_address = p_data_params->client_address;
    char *client_port = p_data_params->client_port;

    enum DataConnMode user_request_mode = *(p_data_params->requested_mode);

    if (user_request_mode == PASV)
    {
        handle_PASV_mode(connect_fd, p_data_fd);
    }
    else if (user_request_mode == PORT)
    {
        /* doesnt have to do anything */
    }
    else
    {
        printf("Data Transfer: Unknown Mode, ask someone why this happened");
    }

    return NULL;
}

/*
 * Handles PASV mode
 * Do not use p_data_fd directly as the main thread might shutdown and clear it
 * Use it to pass back the file descriptors only
 * @returns 0 if fine, -1 if error
 */
int handle_PASV_mode(int connect_fd, struct DataConnFd *p_data_fd)
{

    /* ------------- Get the current address used to connect with client ------------- */
    struct sockaddr client_address; /* interface for the connection with client */
    socklen_t client_length = sizeof(client_address);

    struct AddressPort client_ap;

    getsockname(connect_fd, &client_address, &client_length);
    getnameinfo(&client_address, client_length, client_ap.address, MAXLEN,
                client_ap.port, MAXLEN, NI_NUMERICHOST | NI_NUMERICSERV); /* get info from socket, force numerical values */

    // now create a new socket to listen on that address with arbitrary port
    int pasv_listen_fd = create_listen_socket(client_ap.address, "0");
    p_data_fd->pasv_listen_fd = pasv_listen_fd;
    if (pasv_listen_fd < 0)
    {
        return -1;
    }

    bzero(&client_address, client_length); /* clear address for reuse */
    getsockname(p_data_fd->pasv_listen_fd, &client_address, &client_length);
    getnameinfo(&client_address, client_length, client_ap.address, MAXLEN,
                client_ap.port, MAXLEN, NI_NUMERICHOST | NI_NUMERICSERV); /* get info from socket, force numerical values */

    // information is now updated into client_ap, change to ftp format and write
    char ftp_address[BUFF_SIZE];
    bzero(ftp_address, BUFF_SIZE);

    // first send human readable format
    write(connect_fd, "227-PASV address is ", 20);
    write(connect_fd, client_ap.address, strlen(client_ap.address));
    write(connect_fd, ":", 1);
    write(connect_fd, client_ap.port, strlen(client_ap.port));
    write(connect_fd, "\015\012", 2);

    write(connect_fd, "227 =", 5);

    to_ftp_address_port(client_ap, ftp_address); /* host port string in the format of ftp */
    write(connect_fd, ftp_address, strlen(ftp_address));
    write(connect_fd, "\015\012", 2);

    // finished sending port, no longer need address information
    bzero(&client_address, client_length);

    int pasv_conn_fd;
    if ((pasv_conn_fd = accept(p_data_fd->pasv_listen_fd, &client_address, &client_length)) < 0)
    {
        // accept shutdown by main thread, close and quit
        close(pasv_listen_fd);
        return -1;
    }
    /* -------------- Successfully connected to something --------------*/
    p_data_fd->pasv_conn_fd = pasv_conn_fd;
    close(p_data_fd->pasv_listen_fd); /* close listen socket */
    p_data_fd->pasv_listen_fd = -1;   /* clear listen socket */
    return 0;
}

/*
 * Handles PORT mode when there needs to be a connection
 * @returns 0 if fine, -1 if error
 */
int handle_PORT_mode(int connect_fd, struct DataConnFd *p_data_fd, char *client_address, char *client_port)
{
    // doesnt have to do anything since connection is done after RETR or STOR
    return 0;
}

/*
 * Process commands in Idle state
 * @param request the command that user typed in
 * @param connect_fd the file descriptor of the socket
 * @param p_data_transfer_mode points to the DataConnMode shared by handle_client
 * @param p_transfer_mode_lock mutex lock shared by handle_client
 * @returns Next state
 */
enum ClientState process_idle_mode(struct ClientRequest request, int connect_fd, enum DataConnMode *p_current_mode,
                                   struct DataConnFd *p_data_fd)
{

    if (isEqual(request.verb, "CLOSE"))
    {
        // safe to close since all of the sockets are idle
        close_all_fd(p_data_fd);
    }
    else if (isEqual(request.verb, "PORT"))
    {
        struct AddressPort client_ap = parse_address_port(request.parameter);
        if (isEmpty(client_ap.address))
        {
            char *resp_msg = "501 PORT parameter in incorrect format.\015\012";
            write(connect_fd, resp_msg, strlen(resp_msg));
        }
        else
        {
            // create struct to pass in parameters
            *p_current_mode = PORT;

            struct DataConnParams params;
            params.conn_fd = connect_fd;
            params.data_fd = p_data_fd;
            params.requested_mode = p_current_mode;
            strcpy(params.client_address, client_ap.address);
            strcpy(params.client_port, client_ap.port);

            pthread_t conn_thread;
            pthread_create(&conn_thread, NULL, handle_data_transfer, &params);
            return Idle;
        }
    }
    else if (isEqual(request.verb, "PASV"))
    {
        if (!isEmpty(request.parameter))
        {
            // PASV mode should have empty parameter
            char *resp_msg = "501 PASV should have empty parameter\015\012";
            write(connect_fd, resp_msg, strlen(resp_msg));
        }
        else
        {
            // create struct to pass in parameters
            *p_current_mode = PASV;

            struct DataConnParams params;
            params.conn_fd = connect_fd;
            params.data_fd = p_data_fd;
            params.requested_mode = p_current_mode;

            pthread_t conn_thread;
            pthread_create(&conn_thread, NULL, handle_data_transfer, &params);
            return Idle;
        }
    }
    else if (isEqual(request.verb, "QUIT"))
    {
        char *resp_msg = "221 Closing Connection, Goodbyte and goodbit.\015\012";
        write(connect_fd, resp_msg, strlen(resp_msg));
        return Exit;
    }
    else
    {
        char *resp_msg = "500 Unknown Command.\015\012";
        write(connect_fd, resp_msg, strlen(resp_msg));
    }
    return Idle;
}