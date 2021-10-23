#include "utils.h"

/*
 * First checks if \n exits already exits in outer buffer, if so then push it to buffer and notify to process (ret = 1) 
 * if \n doesnt exist then reads from socket to last_read, if \n doesnt exist and read nothing from socket, can discard (0)
 * otherwise wait for next round of read (-1)
 * DO NOT USE return value as measurement of how many bytes read in buffer!!!!!!
 * @param fd file data for socket
 * @param buffer place to put newline
 * @param buffer_size just set to BUFFSIZE
 * @param outer_buffer the start of buffer that records string that have not been read
 * @param p_last_read pointer to last location 
 * @returns 0 if there are no strings left and socket closed, 1 if buffer is ready to process, -1 if 
 * there still is things to read, but there is nothing to process in buffer
 */
int read_buffer(int fd, char *buffer, size_t buffer_size, char *outer_buffer, char **p_last_read)
{

    char *last_read = *p_last_read;

    // scan outer buffer for \n
    int index = 0, outer_buffer_size = last_read - outer_buffer;
    while (index < outer_buffer_size)
    {
        if (outer_buffer[index] == '\n')
        {
            break;
        }
        index++;
    }
    if (index < outer_buffer_size)
    { /* found a \n at outer_buffer[index] */

        // copy outer_buffer[0]...outer_buffer[index] to buffer
        strncpy(buffer, outer_buffer, index + 1);

        // move outer_buffer[index+1] .. last_read to outer_buffer[0]... (last_read-outer_buffer - index - 1)
        char temp_buffer[buffer_size];
        bzero(temp_buffer, 0);

        int new_outer_buffer_len = (last_read - outer_buffer - index - 1);
        strncpy(temp_buffer, outer_buffer + index + 1, new_outer_buffer_len);
        strncpy(outer_buffer, temp_buffer, buffer_size); /* use buffer_size to clear out outer_buffer in the process */
        *p_last_read = outer_buffer + new_outer_buffer_len;
        return 1;
    }
    // else there was no \n in outer_buffer

    size_t bytes_read;
    bytes_read = read(fd, outer_buffer, BUFF_SIZE - (last_read - outer_buffer));
    *p_last_read += bytes_read;
    if (bytes_read == 0)
    {
        // no \n found and socket closed (read returned 0), this means the outer buffer can be safely discarded
        return 0;
    }
    return -1; /* there might be more input from tcp socket, wait some more, tell the process to wait for another read */
}

/*
 * Creates a listening socket on port. Many thanks to CSAPP Section 11.4.
 * @param host String for host, if any then set = NULL
 * @param port String for port
 * @returns listen_fd file descriptor, or -1 if error
*/
int create_listen_socket(char *host, char *port)
{
    struct addrinfo hints, *results, *p; /* hints to fine tune result list */
    int listen_fd, opt = 1;

    /* ----------- Get address linked list ----------- */

    bzero(&hints, sizeof(struct addrinfo));

    hints.ai_family = AF_INET;                                    /* only IPV-4 for now */
    hints.ai_socktype = SOL_SOCKET;                               /* tcp socket */
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG | AI_NUMERICSERV; /* intend for bind */

    getaddrinfo(host, port, &hints, &results);

    for (p = results; p != NULL; p = p->ai_next)
    {
        // starts trying them
        listen_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listen_fd < 0)
        {
            printf("System: ERROR Socket Creation Failed\n");
            continue;
        }

        // allow binding to already in use addresses
        int socket_opt_ret = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); /* set options */
        if (socket_opt_ret < 0)
        {
            printf("System: ERROR Setting Up Socket Option Failed\n");
            continue;
        }

        int bind_ret = bind(listen_fd, p->ai_addr, p->ai_addrlen); /* bind to address */
        if (bind_ret < 0)
        {
            printf("System: ERROR Binding Failed\n");
            continue;
        }

        if (bind_ret == 0)
        { /* bind success */
            break;
        }
        else
        { /* close and go to next one */
            close(listen_fd);
        }
    }

    freeaddrinfo(results); /* release linked list memory */
    if (p == NULL)
    { /* unable to bind to any <=> traversed to end */
        return -1;
    }

    int listen_ret = listen(listen_fd, MAX_USER_QUEUE); /* start listening */
    if (listen_ret)
    {
        printf("System: ERROR Listen Failed\n");
        close(listen_fd);
        return -1;
    }
    return listen_fd;
}

/*
 * Creates a connection (client) socket to hostname and port. Many thanks to CSAPP Section 11.4.
 * Uses ip independent getaddrinfo to connect.
 * Be aware, this function is blocking
 * @param Port String for port
 * @returns client_fd file descriptor, or -1 if error
*/
int create_connect_socket(char *hostname, char *port)
{
    int client_fd = -1;
    struct addrinfo hints, *results, *p;

    bzero(&hints, sizeof(struct addrinfo));

    hints.ai_socktype = SOCK_STREAM; /* tcp */
    hints.ai_flags = AI_NUMERICSERV | AI_ADDRCONFIG;
    getaddrinfo(hostname, port, &hints, &results);

    for (p = results; p != NULL; p = p->ai_next)
    {
        if ((client_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
        {
            // failed
            continue;
        }

        if (connect(client_fd, p->ai_addr, p->ai_addrlen) != -1) /* no errors in connecting */
        {
            break;
        }
    }

    freeaddrinfo(results);

    if (p == NULL)
    {
        // didnt find a connectable
        return -1;
    }
    else
    {
        return client_fd;
    }
}

/*
 * Changes global variable g_current_server_params
 * TODO: take in argc values and produce dir location and port value
 * @returns -1 if error, 0 if fine
 */
int parse_args(char **argv)
{
    strcpy(g_current_server_params.port, DEFAULT_PORT);

    bzero(g_current_server_params.root_directory, MAXLEN);
    char *path = realpath("/tmp/", g_current_server_params.root_directory);
    if (path != g_current_server_params.root_directory) /* specified by realpath */
    {
        return -1;
    }
    return 0;
}
/*
 * Checks if is prefix, returns 1 if true, 0 if false
 */
int isPrefix(char *string, char *prefix)
{
    if (strncmp(prefix, string, strlen(prefix)) == 0)
    {
        return 1;
    }
    return 0;
}

/*
 * Checks if suffix, returns 1 if true, 0 if false
 */
int isSuffix(char *string, char *suffix)
{
    size_t suffix_len = strlen(suffix), string_len = strlen(string);
    if (suffix_len > string_len)
    {
        return 0;
    }
    else if (strncmp(string + string_len - suffix_len, suffix, suffix_len) == 0)
    {
        return 1;
    }
    return 0;
}

/*
 * Checks if c is an ascii alphabet
 * @returns 1 if true, 0 if false
 */
int isAlphabet(char c)
{
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
    {
        return 1;
    }
    return 0;
}

/*
 * Checks if c is an ascii numeric alphabet
 * @returns 1 if true, 0 if false
 */
int isNumeric(char c)
{
    if (c >= '0' && c <= '9')
    {
        return 1;
    }
    return 0;
}

/*
 * @returns 1 if string is empty, 0 otherwise
 */
int isEmpty(char *string)
{
    if (string[0] == 0)
    {
        return 1;
    }
    return 0;
}

/*
 * @returns 1 if string_a === string_b
 */
int isEqual(char *string_a, char *string_b)
{
    if (strcmp(string_a, string_b) == 0)
    {
        return 1;
    }
    return 0;
}

/*
 * TODO, add security measures for when buffer contains verb/param > MAXLEN
 * @returns struct ClientRequest, if in wrong format then ClientRequest has empty strings
 */
struct ClientRequest parse_request(char *buffer)
{
    struct ClientRequest request;
    bzero(request.verb, MAXLEN);
    bzero(request.parameter, MAXLEN);

    // for security reasons, last char of buffer should == 0
    if (buffer[BUFF_SIZE - 1] != 0)
    {
        // refuse to parse
        printf("Parser: Security Warning, Buffer Overflow, refused to parse command\n");
        return request;
    }

    // check suffix
    if (!isSuffix(buffer, "\015\012"))
    {
        printf("Parser: Client Command Wrong Suffix\n");
        return request;
    }

    int verb_length = 0, index = 0;

    while (index < BUFF_SIZE)
    {
        if (verb_length == 0)
        {
            // currently parsing verb
            if (buffer[index] == ' ')
            {
                // move onto parameter
                verb_length = index;
            }
            else if (buffer[index] == '\015' || buffer[index] == '\012')
            {
                // ended without parameter (fine)
                return request;
            }
            else if (!isAlphabet(buffer[index]))
            {
                // not in correct format, return empty struct
                printf("Parser: Verb Not All Alphabet\n");
                bzero(request.verb, MAXLEN);
                bzero(request.parameter, MAXLEN);
                return request;
            }
            else
            {
                request.verb[index] = buffer[index];
            }
        }
        else
        {
            // currently parsing parameter
            if (buffer[index] == '\015' || buffer[index] == '\012')
            {
                return request;
            }
            else
            {
                request.parameter[index - verb_length - 1] = buffer[index];
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
 * Initialize data connection file descriptors
 * @param p_data_fd pointer to file descriptor fd
 */
void init_dataconn_fd(struct DataConnFd *p_data_fd)
{
    p_data_fd->pasv_conn_fd = -1;
    p_data_fd->pasv_listen_fd = -1;
    p_data_fd->port_conn_fd = -1;
}

/*
 * Parses address and port from FTP specifications
 * TODO: Make this safer by checking port size
 * @param buffer string of size BUFFSIZE to parse
 * @returns parsed address and port, is empty if parse error
 */
struct AddressPort parse_address_port(char *buffer)
{
    struct AddressPort client_in;
    bzero(client_in.address, MAXLEN);
    bzero(client_in.port, MAXLEN);

    if (buffer[BUFF_SIZE - 1] != 0)
    {
        // for security reasons
        return client_in;
    }

    int index = 0, comma_count = 0, port_num_1 = 0, port_num_2 = 0;
    int incorrect_format = 0;
    while (index < BUFF_SIZE)
    {
        if (buffer[index] == 0)
        {
            // end of string
            break;
        }
        if (comma_count <= 3)
        {
            // still processing address
            if (buffer[index] == ',')
            {
                if (index == 0 || buffer[index - 1] == ',')
                {
                    // must have value between ,
                    incorrect_format = 1;
                    break;
                }
                if (comma_count != 3)
                {
                    // dont add in last comma
                    client_in.address[index] = '.';
                }
                comma_count++;
            }
            else if (isNumeric(buffer[index]))
            {
                client_in.address[index] = buffer[index];
            }
            else
            {
                incorrect_format = 1;
                break;
            }
            index++;
        }
        else if (comma_count <= 5)
        {
            // processing port
            if (buffer[index] == ',')
            {
                if (buffer[index - 1] == ',')
                {
                    // must have value between ,
                    incorrect_format = 1;
                    break;
                }
                comma_count++;
            }
            else if (isNumeric(buffer[index]))
            {
                if (comma_count == 4)
                {
                    port_num_1 *= 10;
                    port_num_1 += buffer[index] - '0';
                }
                else
                {
                    // on second part of port
                    port_num_2 *= 10;
                    port_num_2 += buffer[index] - '0';
                }
            }
            else
            {
                incorrect_format = 1;
                break;
            }
            index++;
        }
        else
        {
            // too many commas
            incorrect_format = 1;
            break;
        }
    }
    if (incorrect_format || comma_count != 5 || (index > 0 && buffer[index - 1] == ','))
    { /* cannot end in comma */
        printf("Parser: Parameter not in correct format\n");
        bzero(client_in.address, MAXLEN);
        bzero(client_in.port, MAXLEN);
    }
    else
    {
        sprintf(client_in.port, "%d", port_num_1 * 256 + port_num_2);
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
void to_ftp_address_port(struct AddressPort ap, char *ftp_output)
{
    int index = 0, port_num = 0, pindex = 0;
    while (index < BUFF_SIZE) /* transfer address in ap to ftp_output*/
    {
        if (ap.address[index] == 0)
        {
            break;
        }
        else if (ap.address[index] == '.')
        {
            ftp_output[index] = ',';
        }
        else
        {
            ftp_output[index] = ap.address[index];
        }
        index++;
    }
    ftp_output[index] = ',';
    index++;

    port_num = atoi(ap.port);

    char p1[MAXLEN], p2[MAXLEN];
    bzero(p1, MAXLEN);
    bzero(p2, MAXLEN);

    sprintf(p1, "%d", port_num / 256);
    sprintf(p2, "%d", port_num % 256);

    while (pindex < MAXLEN)
    {
        if (p1[pindex] == 0)
        {
            break;
        }
        else
        {
            ftp_output[index] = p1[pindex];
        }
        index++;
        pindex++;
    }
    ftp_output[index] = ',';
    index++;
    pindex = 0;
    while (pindex < MAXLEN)
    {
        if (p2[pindex] == 0)
        {
            break;
        }
        else
        {
            ftp_output[index] = p2[pindex];
        }
        index++;
        pindex++;
    }
}

/*
 * Check that path exists and output the absolute path to the input
 * Checks that the input doesnt have ../ as to not go up the root directory
 * @params should_create if file doesnt exists, should the function create the file, 1 == create?
 * @returns 1 if path doesnt exists, 2 if path has two dots, -1 if not in correct format/error 
 * 0 if all is well
 */
int get_abspath(char *input_path, char *output_path, int should_create)
{
    // scans path while copying and making sure the path is valid
    char temp_concat[MAXLEN]; // temp variable to store the concated string

    bzero(temp_concat, MAXLEN);
    bzero(output_path, MAXLEN);
    int index = 0, period_count = 0, root_length = 0;
    char *root_dir = g_current_server_params.root_directory;
    while (index < MAXLEN)
    {
        if (root_dir[index] == 0)
        {
            break;
        }
        else
        {
            temp_concat[index] = root_dir[index];
            index++;
            root_length++;
        }
    }
    if (index == MAXLEN)
    {
        // too large
        return -1;
    }

    temp_concat[index] = '/';
    index++;
    int input_path_index = 0;

    while (index < MAXLEN)
    {
        if (input_path[input_path_index] == 0) /* End of string */
        {
            break;
        }

        if (input_path[input_path_index] == '.')
        {
            period_count++;
        }
        else
        {
            period_count = 0;
        }
        if (period_count >= 2)
        {
            // .. not allowed
            return 2;
        }
        temp_concat[index] = input_path[input_path_index];
        index++;
        input_path_index++;
    }
    if (index == MAXLEN)
    {
        // too large
        return -1;
    }

    char *resolved_path = realpath(temp_concat, output_path);
    if (resolved_path != output_path)
    { /* realpath man page specifies = NULL or = output path, just incase */
        /* path not recognized */

        if (should_create == 1)
        {

            // try creating file, safe to do since ../ not in path
            FILE *create_file;
            create_file = fopen(temp_concat, "w");

            if (create_file != NULL)
            {
                // create success
                realpath(temp_concat, output_path);
                return 0;
            }
            // not successfull, return error
        }
        return 1;
    }
    // everything good

    return 0;
}

/*
 * Close all file descriptors that are not == -1, and set the fd to -1
 */
void close_all_fd(struct DataConnFd *p_data_fd)
{
    if (p_data_fd->pasv_conn_fd != -1)
    {
        // safe to close
        close(p_data_fd->pasv_conn_fd);
        p_data_fd->pasv_conn_fd = -1;
    }
    if (p_data_fd->pasv_listen_fd != -1)
    {
        // this means PASV is still accepting on port, shutdown to inform
        shutdown(p_data_fd->pasv_listen_fd, SHUT_RD);
        p_data_fd->pasv_listen_fd = -1;
    }
    if (p_data_fd->port_conn_fd != -1)
    {
        // safe to close
        close(p_data_fd->port_conn_fd);
        p_data_fd->port_conn_fd = -1;
    }
}
