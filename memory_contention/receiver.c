#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>

#include "utils.h"



int main(int argc, char *argv[]){
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <length>\n", argv[0]);
        return 1;
    }

    int length = atoi(argv[1]);
    if (length <= 0) {
        fprintf(stderr, "Invalid length: %s\n", argv[1]);
        return 1;
    }

    Data *data = recv_data(length);
    for(int i = 0; i < data->length; i++){
        printf("%c", data->data[i]);
    }
    printf("\n");
    free(data->data);
    free(data);
    return 0;
}