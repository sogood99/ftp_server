#include "utils.h"

void check_error(int ret_val, char* error_msg){
    // Prints error if ret value == -1
    if (ret_val < 0){
        printf("%s", error_msg);
        exit(EXIT_FAILURE);
    }
}

void parse_args(char** argv){
    // Changes g_current_server_params
    // TODO: take in argc values and produce dir location and port value
    g_current_server_params.port = 8080;

    char* path = realpath("/tmp/", NULL);
    strcpy(g_current_server_params.root_directory, path);

    free(path); /* specified by realpath */
}

int isPrefix(char* string, char* prefix){
    // Checks if prefix, returns 1 if true, 0 if false
    if (strncmp(prefix, string, strlen(prefix)) == 0){
        return 1;
    }
    return 0;
}

int isSuffix(char* string, char* suffix){
    // Checks if suffix, returns 1 if true, 0 if false
    size_t suffix_len = strlen(suffix), string_len = strlen(string);
    if (suffix_len > string_len){
        return 0;
    }else if (strncmp(string + string_len - suffix_len, suffix, suffix_len) == 0){
        return 1;
    }
    return 0;
}