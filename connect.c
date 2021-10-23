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
    struct DataConnFd *p_data_fd = malloc(sizeof(struct DataConnFd));
    init_dataconn_fd(p_data_fd);

    // initialize p_params
    struct DataConnParams *p_params = malloc(sizeof(struct DataConnParams));
    p_params->conn_fd = connect_fd;
    p_params->requested_mode = NOTSET;
    p_params->p_data_fd = p_data_fd;
    bzero(p_params->client_address, MAXLEN);
    bzero(p_params->client_port, MAXLEN);

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
                current_state = process_select_mode(request_command, p_params);
                break;
            case Idle:
                current_state = process_idle_mode(request_command, p_params);
                break;
            default:
                printf("Connector: Warning, switch statement should include all the states\n");
                current_state = Exit;
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
    close_all_fd(p_data_fd);
    printf("Connection Closed\n");
    close(connect_fd);
    free(p_data_fd);
    free(p_params);
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
enum ClientState process_select_mode(struct ClientRequest request, struct DataConnParams *p_params)
{
    int connect_fd = p_params->conn_fd;

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

            p_params->requested_mode = PORT;
            strcpy(p_params->client_address, client_ap.address);
            strcpy(p_params->client_port, client_ap.port);

            char resp_msg[3 * MAXLEN];
            sprintf(resp_msg, "200 PORT Mode Established With %s:%s.\015\012", client_ap.address,
                    client_ap.port);
            write(connect_fd, resp_msg, strlen(resp_msg));
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
            p_params->requested_mode = PASV;

            pthread_t conn_thread;
            pthread_create(&conn_thread, NULL, handle_data_transfer, p_params);
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
 * Starts connecting/listening, use this function only when need connection 
 * @param p_params is of type DataConnParams*
 * @returns Nothing
 */
void *handle_data_transfer(void *p_params)
{
    struct DataConnParams *p_data_params = p_params; /* for typing purposes */

    // get parameters out
    int connect_fd = p_data_params->conn_fd;
    struct DataConnFd *p_data_fd = p_data_params->p_data_fd;
    enum DataConnMode user_request_mode = p_data_params->requested_mode;
    char *client_hostname = p_data_params->client_address;
    char *client_port = p_data_params->client_port;

    if (user_request_mode == PASV)
    {
        handle_PASV_mode(connect_fd, p_data_fd);
    }
    else if (user_request_mode == PORT)
    {
        handle_PORT_mode(connect_fd, p_data_fd, client_hostname, client_port);
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
    char resp_msg[BUFF_SIZE + MAXLEN];
    sprintf(resp_msg, "227-PASV address is %s:%s\015\012", client_ap.address, client_ap.port);
    write(connect_fd, resp_msg, strlen(resp_msg));

    // ftp specified address
    to_ftp_address_port(client_ap, ftp_address); /* host port string in the format of ftp */
    sprintf(resp_msg, "227 =%s\015\012", ftp_address);
    write(connect_fd, resp_msg, strlen(resp_msg));

    // finished sending port, no longer need address information, clear for reuse
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
 * Handles PORT mode when there needs to be a connection (use after RETR or STOR)
 * @returns 0 if fine, -1 if error
 */
int handle_PORT_mode(int connect_fd, struct DataConnFd *p_data_fd, char *client_hostname, char *client_port)
{
    close_all_fd(p_data_fd);
    int client_fd = create_connect_socket(client_hostname, client_port);
    if (client_fd == -1)
    {
        return -1;
    }
    // set the DataConnFd if connection successful
    p_data_fd->port_conn_fd = client_fd;
    return 0;
}

/*
 * Process commands in Idle state
 * @param request the command that user typed in 
 * @param p_params->connect_fd the file descriptor of the socket
 * @param p_params->requested_mode the to the DataConnMode requested by user
 * @param p_params->data_fd file descriptors to pass back the different sockets
 * @returns Next state
 */
enum ClientState process_idle_mode(struct ClientRequest request, struct DataConnParams *p_params)
{
    int connect_fd = p_params->conn_fd;
    struct DataConnFd *p_data_fd = p_params->p_data_fd;
    enum DataConnMode current_mode = p_params->requested_mode;

    if (isEqual(request.verb, "CLOSE"))
    {
        // self created command to test the system
        // safe to close since all of the sockets are idle
        close_all_fd(p_data_fd);
        p_params->requested_mode = NOTSET;
        char *resp_msg = "200 Successfully Closed All Connections\015\012";
        write(connect_fd, resp_msg, strlen(resp_msg));
        return SelectMode;
    }
    else if (isEqual(request.verb, "CONNECT"))
    {
        // self created command to test the system
        if (p_params->requested_mode == PORT)
        {
            pthread_t conn_thread;
            pthread_create(&conn_thread, NULL, handle_data_transfer, p_params);

            char *resp_msg = "200 Establishing Connection\015\012";
            write(connect_fd, resp_msg, strlen(resp_msg));
            return Idle;
        }
        else if (p_params->requested_mode == PASV)
        {
            if (p_params->p_data_fd->port_conn_fd != -1)
            {
                char *resp_msg = "200 PASV Connected\015\012";
                write(connect_fd, resp_msg, strlen(resp_msg));
            }
            else
            {
                char *resp_msg = "425 No PASV Connection Was Established\015\012";
                write(connect_fd, resp_msg, strlen(resp_msg));
            }
            return Idle;
        }
        else
        {
            printf("Idle: Unknown Combination\n");
            return SelectMode;
        }
    }
    else if (isEqual(request.verb, "PORT"))
    {
        close_all_fd(p_data_fd); /* close the rest down and clear */
        // the same as in selectmode state
        struct AddressPort client_ap = parse_address_port(request.parameter);
        if (isEmpty(client_ap.address))
        {
            char *resp_msg = "501 PORT parameter in incorrect format.\015\012";
            write(connect_fd, resp_msg, strlen(resp_msg));
        }
        else
        {
            p_params->requested_mode = PORT;

            bzero(p_params->client_address, MAXLEN);
            bzero(p_params->client_port, MAXLEN);
            strcpy(p_params->client_address, client_ap.address);
            strcpy(p_params->client_port, client_ap.port);

            char resp_msg[3 * MAXLEN];
            sprintf(resp_msg, "200 PORT Mode Established With %s:%s.\015\012", client_ap.address,
                    client_ap.port);
            write(connect_fd, resp_msg, strlen(resp_msg));

            return Idle;
        }
    }
    else if (isEqual(request.verb, "PASV"))
    {
        close_all_fd(p_data_fd); /* close the rest down and clear */
        if (!isEmpty(request.parameter))
        {
            // PASV mode should have empty parameter
            char *resp_msg = "501 PASV should have empty parameter\015\012";
            write(connect_fd, resp_msg, strlen(resp_msg));
        }
        else
        {
            // create struct to pass in parameters
            p_params->requested_mode = PASV;

            pthread_t conn_thread;
            pthread_create(&conn_thread, NULL, handle_data_transfer, p_params);
            return Idle;
        }
    }
    else if (isEqual(request.verb, "RETR"))
    {
        if (current_mode == PASV && p_data_fd->pasv_conn_fd == -1) /* tcp not connected yet */
        {
            char *resp_msg = "425 Connection Was Not Established.\015\012";
            write(connect_fd, resp_msg, strlen(resp_msg));

            close_all_fd(p_data_fd);
            p_params->requested_mode = NOTSET;
            return SelectMode;
        }

        // was established / is in port mode
        char *absolute_path = p_params->file_path;
        absolute_path[MAXLEN] = 0; // safety

        int resp = get_abspath(request.parameter, absolute_path, 0);
        if (resp == 0) /* seems good, send file */
        {
            // send file by creating another thread
            pthread_t thread;
            pthread_create(&thread, NULL, send_file, p_params);
            return Transfer;
        }
        else if (resp == -1 || resp == 1)
        {
            char *resp_msg = "451 Could not read from disk.\015\012";
            write(connect_fd, resp_msg, strlen(resp_msg));
        }
        else if (resp == 2)
        {
            char *resp_msg = "550 Permission Denied.\015\012";
            write(connect_fd, resp_msg, strlen(resp_msg));
        }
        // read error, close all fd and return
        close_all_fd(p_data_fd);
        p_params->requested_mode = NOTSET;
        return SelectMode;
    }
    else if (isEqual(request.verb, "STORE")) /* pretty much the same as RETR */
    {

        if (current_mode == PASV && p_data_fd->pasv_conn_fd == -1) /* tcp not connected yet */
        {
            printf("here2\n");

            char *resp_msg = "425 Connection Was Not Established.\015\012";
            write(connect_fd, resp_msg, strlen(resp_msg));

            close_all_fd(p_data_fd);
            p_params->requested_mode = NOTSET;
            return SelectMode;
        }

        // was established / is in port mode
        char *absolute_path = p_params->file_path;
        absolute_path[MAXLEN] = 0; // safety

        int resp = get_abspath(request.parameter, absolute_path, 1); /* create file if doesnt exist */

        if (resp == 0) /* seems good, send file */
        {

            // store file by creating another thread
            pthread_t thread;
            pthread_create(&thread, NULL, store_file, p_params);
            return Transfer;
        }
        else if (resp == -1 || resp == 1)
        {
            char *resp_msg = "451 Could not read from disk.\015\012";
            write(connect_fd, resp_msg, strlen(resp_msg));
        }
        else if (resp == 2)
        {
            char *resp_msg = "550 Permission Denied.\015\012";
            write(connect_fd, resp_msg, strlen(resp_msg));
        }
        // read error, close all fd and return
        close_all_fd(p_data_fd);
        p_params->requested_mode = NOTSET;
        return SelectMode;
    }
    else if (isEqual(request.verb, "QUIT"))
    {
        close_all_fd(p_data_fd);
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

/*
 * Sends file through client's socket
 * @param p_params of type DataConnParam*
 */
void *send_file(void *p_params)
{
    struct DataConnParams *p_data_params = p_params;
    int connect_fd = p_data_params->conn_fd;
    int data_transfer_fd = -1;
    if (p_data_params->requested_mode == PORT)
    {
        // create a connection
        data_transfer_fd = create_connect_socket(p_data_params->client_address,
                                                 p_data_params->client_port);
        if (data_transfer_fd == -1)
        {
            char *resp_msg = "425 Could not establish a connection.\015\012";
            write(connect_fd, resp_msg, strlen(resp_msg));
            return NULL;
        }
        p_data_params->p_data_fd->port_conn_fd = data_transfer_fd;
    }
    else if (p_data_params->requested_mode == PASV)
    {
        data_transfer_fd = p_data_params->p_data_fd->pasv_conn_fd;
    }
    else
    {
        printf("Send File: Unknown Transfer Mode");
        return NULL;
    }

    char *buffer[BUFF_SIZE];
    bzero(buffer, BUFF_SIZE);
    int file_fd = open(p_data_params->file_path, O_RDONLY);
    if (file_fd == -1) /* open error */
    {
        char *resp_msg = "451 Could not read from disk.\015\012";
        write(connect_fd, resp_msg, strlen(resp_msg));
    }
    while (read(file_fd, buffer, BUFF_SIZE) > 0)
    {

        if (write(data_transfer_fd, buffer, BUFF_SIZE) == -1)
        {
            char *resp_msg = "426 Connection Broken.\015\012";
            write(connect_fd, resp_msg, strlen(resp_msg));
        }
        bzero(buffer, BUFF_SIZE);
    }
    char *resp_msg = "226 Transfer Success.\015\012";
    write(connect_fd, resp_msg, strlen(resp_msg));
    close_all_fd(p_data_params->p_data_fd);
    return NULL;
}
/*
 * Sends file through client's socket
 * @param p_params of type DataConnParam*
 */
void *store_file(void *p_params)
{

    struct DataConnParams *p_data_params = p_params;
    int connect_fd = p_data_params->conn_fd;
    int data_transfer_fd = -1;

    if (p_data_params->requested_mode == PORT)
    {
        // create a connection
        data_transfer_fd = create_connect_socket(p_data_params->client_address,
                                                 p_data_params->client_port);

        if (data_transfer_fd < 0)
        {
            char *resp_msg = "425 Could not establish a connection.\015\012";
            write(connect_fd, resp_msg, strlen(resp_msg));
            return NULL;
        }
        p_data_params->p_data_fd->port_conn_fd = data_transfer_fd;
    }
    else if (p_data_params->requested_mode == PASV)
    {
        data_transfer_fd = p_data_params->p_data_fd->pasv_conn_fd;
    }
    else
    {
        printf("Store File: Unknown Transfer Mode");
        return NULL;
    }

    char buffer[BUFF_SIZE];
    bzero(buffer, BUFF_SIZE);
    FILE *p_file = fopen(p_data_params->file_path, "w");
    if (p_file == NULL) /* open error */
    {
        char *resp_msg = "451 Could not read from disk.\015\012";
        write(connect_fd, resp_msg, strlen(resp_msg));
    }
    fclose(p_file);                                /* clear out file */
    p_file = fopen(p_data_params->file_path, "a"); /* open in append mode */
    int read_len;
    while ((read_len = read(data_transfer_fd, buffer, BUFF_SIZE)) > 0)
    {
        fwrite(buffer, 1, read_len, p_file); /* byte mode */
        bzero(buffer, BUFF_SIZE);
    }
    fclose(p_file);
    char *resp_msg = "226 Transfer Success.\015\012";
    write(connect_fd, resp_msg, strlen(resp_msg));
    close_all_fd(p_data_params->p_data_fd);
    return NULL;
}