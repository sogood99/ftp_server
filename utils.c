#include "utils.h"

/*
 * Creates a listening socket on port. Many thanks to CSAPP Section 11.4.
 * @param Port String for port
 * @returns listen_fd file descriptor, or -1 if error
*/
int create_listen_socket(char* port){
    struct addrinfo hints, *results, *p; /* hints to fine tune result list */
    int listen_fd, opt = 1;

    /* ----------- Get address linked list ----------- */

    bzero(&hints, sizeof(struct addrinfo));

    hints.ai_family = AF_INET; /* only IPV-4 for now */
    hints.ai_socktype = SOL_SOCKET; /* tcp socket */
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG | AI_NUMERICSERV;

    getaddrinfo(NULL, port, &hints, &results);

    for (p = results; p != NULL; p = p->ai_next){
        // starts trying them
        listen_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listen_fd < 0){
            printf("System: ERROR Socket Creation Failed\n");
            continue;
        }

        // allow binding to already in use addresses
        int socket_opt_ret = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); /* set options */
        if (socket_opt_ret < 0){
            printf("System: ERROR Setting Up Socket Option Failed\n");
            continue;
        }

        int bind_ret = bind(listen_fd, p->ai_addr, p->ai_addrlen); /* bind to address */
        if (bind_ret < 0) {
            printf("System: ERROR Binding Failed\n");
            continue;
        }

        if (bind_ret == 0){ /* bind success */
            break;
        }else{ /* close and go to next one */
            close(listen_fd);
        }
    }

    freeaddrinfo(results); /* release linked list memory */
    if (p == NULL){/* unable to bind to any <=> traversed to end */
        return -1;
    }

    int listen_ret = listen(listen_fd, MAX_USER_QUEUE); /* start listening */
    if (listen_ret){
        printf("System: ERROR Listen Failed\n");
        close(listen_fd);
        return -1;
    }
    return listen_fd;
}

/*
 * Creates a connection (client) socket to hostname and port. Many thanks to CSAPP Section 11.4.
 * @param Port String for port
 * @returns conn_fd file descriptor, or -1 if error
*/
int create_connect_socket(char* hostname, char* port){
    return -1;
}

/*
 * Changes global variable g_current_server_params
 * TODO: take in argc values and produce dir location and port value
 */
void parse_args(char** argv){
    strcpy(g_current_server_params.port, DEFAULT_PORT);

    char* path = realpath("/tmp/", NULL);
    strcpy(g_current_server_params.root_directory, path);

    free(path); /* specified by realpath */
}
/*
 * Checks if is prefix, returns 1 if true, 0 if false
 */
int isPrefix(char* string, char* prefix){
    if (strncmp(prefix, string, strlen(prefix)) == 0){
        return 1;
    }
    return 0;
}

/*
 * Checks if suffix, returns 1 if true, 0 if false
 */
int isSuffix(char* string, char* suffix){
    size_t suffix_len = strlen(suffix), string_len = strlen(string);
    if (suffix_len > string_len){
        return 0;
    }else if (strncmp(string + string_len - suffix_len, suffix, suffix_len) == 0){
        return 1;
    }
    return 0;
}

/*
 * Checks if c is an ascii alphabet
 * @returns 1 if true, 0 if false
 */
int isAlphabet(char c){
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')){
        return 1;
    }
    return 0;
}

/*
 * Checks if c is an ascii numeric alphabet
 * @returns 1 if true, 0 if false
 */
int isNumeric(char c){
    if (c >= '0' && c <= '9'){
        return 1;
    }
    return 0;
}

/*
 * @returns 1 if string is empty, 0 otherwise
 */
int isEmpty(char* string){
    if (string[0] == 0){
        return 1;
    }
    return 0;
}

/*
 * @returns 1 if string_a === string_b
 */
int isEqual(char* string_a, char* string_b){
    if (strcmp(string_a, string_b) == 0){
        return 1;
    }
    return 0;
}

/*
 * TODO, add security measures for when buffer contains verb/param > MAXLEN
 * @returns struct ClientRequest, if in wrong format then ClientRequest has empty strings
 */
struct ClientRequest parse_request(char* buffer){
    struct ClientRequest request;
    bzero(request.verb, MAXLEN);
    bzero(request.parameter, MAXLEN);

    // for security reasons, last char of buffer should == 0
    if (buffer[BUFF_SIZE-1] != 0){
        // refuse to parse
        printf("Parser: Security Warning, Buffer Overflow, refused to parse command\n");
        return request;
    }

    // check suffix
    if (!isSuffix(buffer, "\015\012")){
        printf("Parser: Client Command Wrong Suffix\n");
        return request;
    }

    int verb_length = 0, index = 0;

    while (index < BUFF_SIZE){
        if (verb_length == 0){
            // currently parsing verb
            if (buffer[index] == ' '){
                // move onto parameter
                verb_length = index;
            } else if (buffer[index] == '\015' || buffer[index] == '\012'){
                // ended without parameter (fine)
                return request;
            } else if (!isAlphabet(buffer[index])){
                // not in correct format, return empty struct
                printf("Parser: Verb Not All Alphabet\n");
                bzero(request.verb, MAXLEN);
                bzero(request.parameter, MAXLEN);
                return request;
            }else{
                request.verb[index] = buffer[index];
            }
        }else{
            // currently parsing parameter
            if (buffer[index] == '\015' || buffer[index] == '\012'){
                return request;
            }else{
                request.parameter[index-verb_length-1] = buffer[index];
            }
        }
        index++;
    }

    // should return before here
    printf("Error in parsing, should not be here, please slap the person who wrote this code (me)");
    bzero(request.verb, MAXLEN);
    bzero(request.parameter, MAXLEN);
    return request;
}

/*
 * Pthread safe way of reading the value of pointer
 * @param p_lock pointer to mutex lock
 * @param p_value pointer to value
 * @returns value in pointer
 */
enum DataConnMode get_connection_mode(pthread_mutex_t* p_lock, enum DataConnMode* p_value){
    pthread_mutex_lock(p_lock);
    enum DataConnMode value = *p_value;
    pthread_mutex_unlock(p_lock);
    return value;
}

/*
 * Pthread safe way of modifying value
 * @param p_lock Mutex Lock
 * @param p_value pointer to containing value
 * @param new_value what to change to
 */
void set_connection_mode(pthread_mutex_t* p_lock, enum DataConnMode* p_value, enum DataConnMode new_value){
    pthread_mutex_lock(p_lock);
    *p_value = new_value;
    pthread_mutex_unlock(p_lock);
}

/*
 * Parses address and port from FTP specifications
 * TODO: Make this safer by checking port size
 * @param buffer string of size BUFFSIZE to parse
 * @returns parsed address and port, is empty if parse error
 */
struct AddressPort parse_address_port(char* buffer){
    struct AddressPort client_in;
    bzero(client_in.address, MAXLEN);
    bzero(client_in.port, MAXLEN);

    if (buffer[BUFF_SIZE-1] != 0){
        // for security reasons
        return client_in;
    }

    int index = 0, comma_count = 0, port_num = 0;
    int incorrect_format = 0;
    while (index < BUFF_SIZE){
        if (buffer[index] == 0){
            // end of string
            break;
        }
        if (comma_count <= 3){
            // still processing address
            if (buffer[index] == ','){
                if (index == 0 || buffer[index-1] == ','){
                    // must have value between ,
                    incorrect_format = 1;
                    break;
                }
                if (comma_count != 3){
                    // dont add in last comma
                    client_in.address[index] = '.';
                }
                comma_count ++;
            }else if (isNumeric(buffer[index])){
                client_in.address[index] = buffer[index];
            }else{
                incorrect_format = 1;
                break;
            }
            index ++;
        }else if (comma_count <= 5){
            // processing port
            if (buffer[index] == ','){
                if (buffer[index-1] == ','){
                    // must have value between ,
                    incorrect_format = 1;
                    break;
                }
                port_num *= 256;
                comma_count ++;
            }else if (isNumeric(buffer[index])){
                port_num += buffer[index]-'0';
            }else{
                incorrect_format = 1;
                break;
            }
            index++;
        }else{
            // too many commas
            incorrect_format = 1;
            break;
        }
    }
    if (incorrect_format || comma_count != 5 || (index > 0 && buffer[index-1] == ',')){ /* cannot end in comma */
        printf("Parser: Parameter not in correct format\n");
        bzero(client_in.address, MAXLEN);
        bzero(client_in.port, MAXLEN);
    }else{
        sprintf(client_in.port, "%d", port_num);
    }
    return client_in;
}

/*
 * Turns address and port to FTP specifications h1,h2,h3,h4,p1,p2 where port = p1*256+p2
 * Implicitly assuming correctness of address and port format (since given by standard libs)
 * @param ap address and port
 * @param ftp_output where to output
 * @returns string of address and port in ftp standards
 */
void to_ftp_address_port(struct AddressPort ap, char* ftp_output){
    int index = 0, port_num = 0, pindex = 0;
    while (index < BUFF_SIZE){
        if (ap.address[index] == 0){
            break;
        } else if (ap.address[index] == '.'){
            ftp_output[index] = ',';
        }else{
            ftp_output[index] = ap.address[index];
        }
        index ++;
    }
    ftp_output[index] = ',';
    index ++;

    port_num = atoi(ap.port);

    char p1[MAXLEN], p2[MAXLEN];
    bzero(p1,MAXLEN);
    bzero(p2,MAXLEN);

    sprintf(p1, "%d", port_num/256);
    sprintf(p2, "%d", port_num%256);
    
    while (pindex < MAXLEN){
        if (p1[pindex] == 0){
            break;
        }else{
            ftp_output[index] = p1[pindex];
        }
        index ++;
        pindex ++;
    }
    ftp_output[index] = ',';
    index ++;
    pindex = 0;
    while (pindex < MAXLEN){
        if (p2[pindex] == 0){
            break;
        }else{
            ftp_output[index] = p2[pindex];
        }
        index ++;
        pindex ++;
    }
}