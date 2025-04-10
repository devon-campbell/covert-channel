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
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <length> <output_file>\n", argv[0]);
        return 1;
    }

    int length = atoi(argv[1]);
    if (length <= 0) {
        fprintf(stderr, "Invalid length: %s\n", argv[1]);
        return 1;
    }

    const char *output_file = argv[2];
    FILE *file = fopen(output_file, "wb");
    if (!file) {
        perror("Failed to open output file");
        return 1;
    }

    Data *data = recv_data(length);
    if (fwrite(data->data, 1, data->length, file) != data->length) {
        perror("Failed to write data to file");
        fclose(file);
        free(data->data);
        free(data);
        return 1;
    }

    fclose(file);
    free(data->data);
    free(data);
    return 0;
}