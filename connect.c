#include "connect.h"

/*
 * To handle each ftp client
 * @param p_connect_fd int pointer to connected socket file descriptor pointer
 */
void *handle_client(void *p_connect_fd)
{
    // for pthreading
    int communication_fd = *((int *)p_connect_fd);
    free(p_connect_fd);

    write(communication_fd, "Hello From Server\r\n", 20);

    // for communication
    char buffer[BUFF_SIZE + 1], outer_buffer[BUFF_SIZE + 1], *last_read = outer_buffer;
    int did_read;                       /* bool if anything is left read from buffer, check out read_buffer function for more info */
    bzero(buffer, BUFF_SIZE + 1);       /* clear buffer */
    bzero(outer_buffer, BUFF_SIZE + 1); /* clear buffer */

    struct ClientRequest request; /* user's request */

    /* -------- FTP Code Begins -------- */

    enum ClientState current_state = Login;

    // for login state
    char current_username[MAXLEN] = {0};
    char current_password[MAXLEN] = {0};

    // for async io (select)
    fd_set socket_set, ready_set;
    FD_ZERO(&socket_set);
    FD_SET(communication_fd, &socket_set);

    // for selectmode state (PASV and PORT data sockets)
    enum DataConnMode current_mode = NOTSET;
    struct DataConnFd data_fd;
    init_dataconn_fd(&data_fd); /* set to all -1 */

    // address for storing to things for socket
    struct sockaddr address;
    socklen_t address_length = sizeof(struct sockaddr);

    // for PORT mode connection
    char PORT_address[MAXLEN];
    char PORT_port[MAXLEN];

    while (1)
    /* starts handleing requests from communication socket or data transfer socket */
    {
        ready_set = socket_set;
        select(FD_SETSIZE, &ready_set, NULL, NULL, NULL); /* blocks until one of the socket interrupts */

        // handles it one at a time
        if (FD_ISSET(communication_fd, &ready_set))
        {
            // handle user commands (read until newline)
            while ((did_read = read_buffer(communication_fd, buffer, BUFF_SIZE, outer_buffer, &last_read)) != 1)
                ;

            if (did_read == 0)
            {
                // communication socket disconnected by client
                close(communication_fd);
                close_all_fd(&data_fd, &socket_set);
                printf("Connection Closed\n");
                return NULL;
            }
            if (did_read == 1)
            {
                /* ----------- Process Command ----------- */
                printf("recieved command: %s", buffer);
                printf("current state: %d\n", current_state);
                parse_request(buffer, &request);

                switch (current_state)
                {
                case Login:
                    if (isEqual(request.verb, "login"))
                    {
                        current_state = SelectMode;
                        write(communication_fd, "Server: Logged In\r\n", 20);
                        printf("Changed to select state\n");
                    }
                    else
                    {
                        write(communication_fd, "Unknown Command\r\n", 18);
                        printf("Unknown command\n");
                    }
                    break;
                case SelectMode:
                    printf("command=%s=\n", request.verb);

                    if (isEqual(request.verb, "pasv"))
                    {
                        current_mode = PASV;
                        // get ip address from communication_socket/fd

                        struct AddressPort client_ap;

                        getsockname(communication_fd, &address, &address_length);
                        getnameinfo(&address, address_length, client_ap.address, MAXLEN,
                                    client_ap.port, MAXLEN, NI_NUMERICHOST | NI_NUMERICSERV);

                        // pass host to listen (since there could be many on 1 computer)
                        data_fd.pasv_listen_fd = create_listen_socket(client_ap.address, "0");

                        // add into select socket
                        FD_SET(data_fd.pasv_listen_fd, &socket_set);

                        // get ip address from listening/fd
                        bzero(&address, address_length);
                        getsockname(data_fd.pasv_listen_fd, &address, &address_length);
                        getnameinfo(&address, address_length, client_ap.address, MAXLEN,
                                    client_ap.port, MAXLEN, NI_NUMERICHOST | NI_NUMERICSERV);

                        write(communication_fd, client_ap.address, strlen(client_ap.address));
                        write(communication_fd, client_ap.port, strlen(client_ap.port));
                        write(communication_fd, "\r\n", 2);
                        printf("sent pasv info\n");
                        current_state = Idle;
                    }
                    else if (isEqual(request.verb, "port"))
                    {
                        current_mode = PORT;

                        struct AddressPort client_ap;
                        parse_address_port(request.parameter, &client_ap);
                        bzero(PORT_address, MAXLEN);
                        bzero(PORT_port, MAXLEN);
                        strcpy(PORT_address, client_ap.address);
                        strcpy(PORT_port, client_ap.port);
                        printf("port: %s:%s\n", PORT_address, PORT_port);
                        write(communication_fd, "recieved port \r\n", 17);
                        current_state = Idle;
                    }
                    else
                    {
                        write(communication_fd, "Unknown Command\r\n", 18);
                        printf("Unknown command\n");
                    }
                    break;
                case Idle:
                    if (isEqual(request.verb, "pasv"))
                    {
                        current_mode = PASV;

                        close_all_fd(&data_fd, &socket_set);
                        struct AddressPort client_ap;

                        getsockname(communication_fd, &address, &address_length);
                        getnameinfo(&address, address_length, client_ap.address, MAXLEN,
                                    client_ap.port, MAXLEN, NI_NUMERICHOST | NI_NUMERICSERV);

                        // pass host to listen (since there could be many on 1 computer)
                        data_fd.pasv_listen_fd = create_listen_socket(client_ap.address, "0");

                        // add into select socket
                        FD_SET(data_fd.pasv_listen_fd, &socket_set);

                        // get ip address from listening/fd
                        bzero(&address, address_length);
                        getsockname(data_fd.pasv_listen_fd, &address, &address_length);
                        getnameinfo(&address, address_length, client_ap.address, MAXLEN,
                                    client_ap.port, MAXLEN, NI_NUMERICHOST | NI_NUMERICSERV);

                        write(communication_fd, client_ap.address, strlen(client_ap.address));
                        write(communication_fd, client_ap.port, strlen(client_ap.port));
                        write(communication_fd, "\r\n", 2);
                        printf("sent pasv info\n");
                        current_state = Idle;
                    }
                    else if (isEqual(request.verb, "port"))
                    {
                        current_mode = PORT;

                        close_all_fd(&data_fd, &socket_set);
                        struct AddressPort client_ap;
                        parse_address_port(request.parameter, &client_ap);
                        bzero(PORT_address, MAXLEN);
                        bzero(PORT_port, MAXLEN);
                        strcpy(PORT_address, client_ap.address);
                        strcpy(PORT_port, client_ap.port);
                        printf("port: %s:%s\n", PORT_address, PORT_port);
                        write(communication_fd, "recieved port \r\n", 17);
                        current_state = Idle;
                    }
                    else if (isEqual(request.verb, "stor"))
                    {
                        if (current_mode == PASV)
                        {
                            // start sending stuff
                        }
                        else if (current_mode == PORT)
                        {
                            // start connecting and sending stuff
                            printf("%s\n", PORT_address);
                            printf("%s\n", PORT_port);
                            data_fd.port_conn_fd = create_connect_socket(PORT_address, PORT_port);
                            printf("survived here");
                            FD_SET(data_fd.port_conn_fd, &socket_set);
                            accept(data_fd.port_conn_fd, &address, &address_length);
                        }
                    }
                    else if (isEqual(request.verb, "retr"))
                    {
                    }
                    else
                    {
                        write(communication_fd, "Unknown Command\r\n", 18);
                        printf("Unknown command\n");
                    }
                    break;
                case Transfer:
                    if (isEqual(request.verb, "port"))
                    {
                        write(communication_fd, "Unknown Command\r\n", 18);
                        printf("Unknown command\n");
                        printf("port\n");
                    }
                    else
                    {
                        write(communication_fd, "Unknown Command\r\n", 18);
                        printf("Unknown command\n");
                    }
                    break;
                default:
                    break;
                }
                if (current_state == Exit)
                {
                    close_all_fd(&data_fd, &socket_set);
                    close(communication_fd);
                    return NULL;
                }
            }
        }
        else if (data_fd.pasv_listen_fd != -1 && FD_ISSET(data_fd.pasv_listen_fd, &ready_set))
        {
            // handle pasv accepts
            printf("pasv listen ready\n");

            // accept connections
            data_fd.pasv_conn_fd = accept(data_fd.pasv_listen_fd, &address, &address_length);

            printf("pasv connected ready\n");
            write(data_fd.pasv_conn_fd, "Hello From Server\r\n", 20);

            // close listening socket
            FD_CLR(data_fd.pasv_listen_fd, &socket_set);
            close(data_fd.pasv_listen_fd);
            data_fd.pasv_listen_fd = -1;

            // add new data_socket to socketset
            FD_SET(data_fd.pasv_conn_fd, &socket_set);
        }
        else if (data_fd.pasv_conn_fd != -1 && FD_ISSET(data_fd.pasv_conn_fd, &ready_set))
        {
            printf("pasv data ready\n");
            write(data_fd.pasv_conn_fd, "hello client\r\n", 15);
            // handle data comming in or out of PASV data socket
            if (current_state == Transfer)
            {
                // process only if tranfer state
            }
        }
        else if (data_fd.port_conn_fd != -1 && FD_ISSET(data_fd.port_conn_fd, &ready_set))
        {
            printf("port data ready\n");

            // handle connection/data comming in or out of PORT socket
            if (current_state == Transfer)
            {
                // process only if tranfer state
                write(data_fd.port_conn_fd, "hello world\r\n", 14);
                close_all_fd(&data_fd, &socket_set);
            }
            else
            {
                printf("should not happen\n");
            }
        }
        else
        {
            printf("Not sure what happened\n");
        }
    }
}