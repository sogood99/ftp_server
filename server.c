/* main server file, primarily handles initializing server and initial connection with client */
#include "utils.h"
#include "connect.h"

/* 
* Creates a listening socket and starts assigning work to threads. 
* Runs indefinitely.
*/
void run_server()
{
    int listen_fd, conn_fd;                         /* listen fd and (client)connection fd */
    struct sockaddr server_address, client_address; /* address for listen socket and (client)connection socket */
    socklen_t server_length, client_length = sizeof(struct sockaddr_storage);
    char hostname_str[MAXLEN], port_str[MAXLEN]; /* used for both server and client */

    listen_fd = create_listen_socket("0.0.0.0", g_current_server_params.port); /* creates a listening socket */
    if (listen_fd < 0)
    {
        exit(EXIT_FAILURE);
    }

    /* ------------- Socket Listen Successful ------------- */

    getsockname(listen_fd, &server_address, &server_length);

    printf("System: Everything OK!\nSystem: Starting Server on Root Directory = ");
    printf("%s, ", g_current_server_params.root_directory);

    getnameinfo(&server_address, server_length, hostname_str, MAXLEN,
                port_str, MAXLEN, NI_NUMERICHOST | NI_NUMERICSERV); /* get info from socket, force numerical values */
    printf("Address = %s ", hostname_str);
    printf("Port = %s\n", port_str);

    while (1)
    {
        // keep on accepting connection from client
        conn_fd = accept(listen_fd, &client_address, &client_length);
        getnameinfo(&client_address, client_length, hostname_str, MAXLEN,
                    port_str, MAXLEN, NI_NUMERICHOST | NI_NUMERICSERV);
        printf("System: Connection Established With %s:%s\n", hostname_str, port_str);

        // new thread to handle each client
        pthread_t conn_thread;
        int *p_client_for_thread = malloc(sizeof(int));
        *p_client_for_thread = conn_fd;
        pthread_create(&conn_thread, NULL, handle_client, p_client_for_thread);
    }
    close(listen_fd);
    return;
}

int main(int argc, char **argv)
{
    if (parse_args(argv) == -1)
    {
        printf("System: Root Directory Not Found\n");
        return EXIT_FAILURE;
    }
    run_server();
    return 0;
}
