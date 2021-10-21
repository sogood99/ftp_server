/* main server file */
#include "connect.h"
#include "utils.h"

void run_server(){
    int listen_fd, conn_fd; /* listen fd and (client)connection fd */
    struct sockaddr_in address, client_address; /* address for listen socket and (client)connection socket */
    socklen_t client_length = sizeof(struct sockaddr_storage);
    char client_hostname[MAXLEN], client_port[MAXLEN];
    int opt = 1;

    listen_fd = socket(AF_INET, SOCK_STREAM, 0); /* tcp socket with default protocol */
    check_error(listen_fd, "ERROR: Socket Creation Failed");

    int socket_opt_ret = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); /* set options */
    check_error(socket_opt_ret, "ERROR: Setting up Socket Option Failed");

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(g_current_server_params.port);

    int bind_ret = bind(listen_fd, (SA *)&address, sizeof(address)); /* bind to port */
    check_error(bind_ret, "ERROR: Binding Failed");

    int listen_ret = listen(listen_fd, MAX_USER_QUEUE); /* start listening */
    check_error(listen_ret, "ERROR: Listen Failed");

    printf("Everything OK!\nStarting Server on Root Directory: ");
    printf("%s\n", g_current_server_params.root_directory);
    printf("Port: %d\n", g_current_server_params.port);

    while (1){
        /* keep on accepting connection from client */
        int conn_fd = accept(listen_fd, (SA *)&client_address, &client_length);
        getnameinfo((SA*)&client_address, client_length, client_hostname, MAXLEN,
            client_port, MAXLEN, 0);
        printf("Connection Established With %s:%s\n", client_hostname, client_port);

        pthread_t conn_thread;
        int *p_client_for_thread = malloc(sizeof(int));
        *p_client_for_thread = conn_fd;
        pthread_create(&conn_thread, NULL, handle_client, p_client_for_thread);
    }

    close(listen_fd);
    return;
}

int main(int argc, char** argv){
    parse_args(argv);
    run_server();
    return 0;
}
