#include "utils.h"

void check(int ret_val, char* error_msg){
    /* Prints error if error value == -1 */
    if (ret_val < 0){
        printf("%s", error_msg);
        exit(EXIT_FAILURE);
    }
}

int parse_args(char** argv){
    // TODO: take in argc values and produce dir location and port value
    return 8080;
}