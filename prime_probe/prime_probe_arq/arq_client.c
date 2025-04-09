#define _POSIX_C_SOURCE 200809L

#include "arq_channel.h"
#include "phy.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];

    FILE *input_file = fopen(filename, "r");
    if (!input_file) {
        perror("fopen input_file");
        return 1;
    }

    // Determine file size
    fseek(input_file, 0, SEEK_END);
    size_t len = ftell(input_file);
    rewind(input_file);

    char *bytestream = malloc(len + 1);
    size_t bytes_read = fread(bytestream, 1, len, input_file);
    if (bytes_read != len) {
        fprintf(stderr, "Warning: Expected to read %zu bytes, but got %zu\n", len, bytes_read);
    }

    bytestream[len] = '\0';
    fclose(input_file);

    // Setup: Prime+Probe + threshold
    phy_init();

    // Save sent message to log
    FILE *sent_file = fopen("client_sent.txt", "w");
    FILE *time_file = fopen("time.txt", "w");
    fprintf(sent_file, "%s", bytestream);
    fclose(sent_file);

    bool seq_num = 0;

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);  // START TIMING

    for (size_t i = 0; i < len; i++) {
        // printf("\nSending byte: %c (0x%x)\n", bytestream[i], bytestream[i]);
        transmit_arq_frame((uint8_t)bytestream[i], seq_num);
        seq_num ^= 1;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);  // END TIMING

    double elapsed_time = (end.tv_sec - start.tv_sec) +
                          (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("[client] Done sending. Elapsed time: %.3f seconds\n", elapsed_time);
    fprintf(time_file, "%f", elapsed_time);
    fclose(time_file);

    free(bytestream);
    return 0;
}