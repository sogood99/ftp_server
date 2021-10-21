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