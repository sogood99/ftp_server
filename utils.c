#include "utils.h"

/*
    Prints error if ret value == -1
*/
void check_error(int ret_val, char* error_msg){
    if (ret_val < 0){
        printf("%s", error_msg);
        exit(EXIT_FAILURE);
    }
}

/*
    Changes g_current_server_params
    TODO: take in argc values and produce dir location and port value
*/
void parse_args(char** argv){
    g_current_server_params.port = DEFAULT_PORT;

    char* path = realpath("/tmp/", NULL);
    strcpy(g_current_server_params.root_directory, path);

    free(path); /* specified by realpath */
}
/*
    Checks if is prefix, returns 1 if true, 0 if false
*/
int isPrefix(char* string, char* prefix){
    if (strncmp(prefix, string, strlen(prefix)) == 0){
        return 1;
    }
    return 0;
}

/*
    Checks if suffix, returns 1 if true, 0 if false
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
    Checks if c is an ascii alphabet
    @returns 1 if true, 0 if false
*/
int isAlphabet(char c){
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')){
        return 1;
    }
    return 0;
}

/*
    @return 1 if string is empty, 0 otherwise
*/
int isEmpty(char* string){
    if (string[0] == 0){
        return 1;
    }
    return 0;
}

/*
    @return 1 if string_a === string_b
*/
int isEqual(char* string_a, char* string_b){
    if (strcmp(string_a, string_b) == 0){
        return 1;
    }
    return 0;
}

/*
    TODO, add security measures for when buffer contains verb/param > MAXLEN
    @return struct ClientRequest, if in wrong format then ClientRequest has empty strings
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
