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



int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s file\n", argv[0]);
        return 1;
    }

    FILE *file = fopen(argv[1], "rb");
    if (!file) {
        perror("Failed to open file");
        return 1;
    }

    fseek(file, 0, SEEK_END);
    int n = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *data = (char *)malloc(n);
    if (!data) {
        perror("Failed to allocate memory");
        fclose(file);
        return 1;
    }

    if (fread(data, 1, n, file) != n) {
        perror("Failed to read file");
        free(data);
        fclose(file);
        return 1;
    }

    fclose(file);
    printf("Data length: %d\n", n);
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    if (send_data(data, n) < 0) {
        fprintf(stderr, "Failed to send data\n");
        free(data);
        return 1;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("Data sent successfully.\n");
    printf("Time taken to send data: %.3f seconds\n", elapsed_time);
    printf("Data size: %d bytes\n", n);
    printf("Transfer rate: %.3f bytes/second\n", n / elapsed_time);
    
    free(data);
    return 0;
}