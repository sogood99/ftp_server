/* following used for ide completion (delete during production) */
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <pthread.h>
#include <unistd.h>

/* main server file */
#include "utils.h"

void run_server(int port){
    int listen_fd, conn_fd; /* listen fd and (client)connection fd */
    struct sockaddr_in address, client_address; /* address for listen socket and (client)connection socket */
    socklen_t client_length = sizeof(struct sockaddr_storage);
    char client_hostname[MAXLEN], client_port[MAXLEN];
    int opt = 1;

    listen_fd = socket(AF_INET, SOCK_STREAM, 0); /* tcp socket with default protocol */
    check(listen_fd, "ERROR: Socket Creation Failed");

    int socket_opt_ret = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); /* set options */
    check(socket_opt_ret, "ERROR: Setting up Socket Option Failed");

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);

    int bind_ret = bind(listen_fd, (SA *)&address, sizeof(address)); /* bind to port */
    check(bind_ret, "ERROR: Binding Failed");

    int listen_ret = listen(listen_fd, MAX_USER_QUEUE); /* start listening */
    check(listen_ret, "ERROR: Listen Failed");

    printf("Everything OK!\n");

    while (1){
        int conn_fd = accept(listen_fd, (SA *)&client_address, &client_length);
        getnameinfo((SA*)&client_address, client_length, client_hostname, MAXLEN,
            client_port, MAXLEN, 0);
        printf("Connection Established With %s:%s\n", client_hostname, client_port);
        handle_client(conn_fd);
    }

    close(listen_ret);
    return;
}

int main(int argc, char** argv){
    int port = parse_args(argv);
    run_server(port);
    return 0;
}