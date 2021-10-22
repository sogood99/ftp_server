/* main server file, primarily handles initializing server and initial connection with client */
#include "connect.h"
#include "utils.h"

void run_server(){
    int listen_fd, conn_fd; /* listen fd and (client)connection fd */
    struct sockaddr_in server_address, client_address; /* address for listen socket and (client)connection socket */
    socklen_t server_length, client_length = sizeof(struct sockaddr_storage);
    char hostname_str[MAXLEN], port_str[MAXLEN]; // used for both server and client
    int opt = 1;

    listen_fd = socket(AF_INET, SOCK_STREAM, 0); /* tcp socket with default protocol */
    check_error(listen_fd, "System: ERROR Socket Creation Failed");

    int socket_opt_ret = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); /* set options */
    check_error(socket_opt_ret, "System: ERROR Setting Up Socket Option Failed");

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(DEFAULT_IP);
    server_address.sin_port = htons(g_current_server_params.port);

    server_length = sizeof(server_address); /* set length */

    int bind_ret = bind(listen_fd, (SA *)&server_address, server_length); /* bind to port */
    check_error(bind_ret, "System: ERROR Binding Failed");

    int listen_ret = listen(listen_fd, MAX_USER_QUEUE); /* start listening */
    check_error(listen_ret, "System: ERROR Listen Failed");

    /* ------------- Socket Listen Successful ------------- */

    printf("System: Everything OK!\nSystem: Starting Server on Root Directory = ");
    printf("%s, ", g_current_server_params.root_directory);

    getnameinfo((SA*)&server_address, server_length, hostname_str, MAXLEN,
        port_str, MAXLEN, NI_NUMERICHOST|NI_NUMERICSERV); /* get info, force numerical (string) values */
    printf("Address = %s ", hostname_str);
    printf("Port = %s\n", port_str);

    while (1){
        // keep on accepting connection from client
        conn_fd = accept(listen_fd, (SA *)&client_address, &client_length);
        getnameinfo((SA*)&client_address, client_length, hostname_str, MAXLEN,
            port_str, MAXLEN, NI_NUMERICHOST|NI_NUMERICSERV);
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

int main(int argc, char** argv){
    parse_args(argv);
    run_server();
    return 0;
}
