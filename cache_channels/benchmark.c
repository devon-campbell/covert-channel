#define _GNU_SOURCE
#include "arq_protocol.h"

#include <stdio.h>
#include <stdlib.h>


// Usage: ./benchmark <filename>
int main(int argc, char *argv[])
{
    char *input_fname;


    if (argc == 2)
    {
        input_fname = argv[1];
    }
    else
    {
        printf("Usage: %s <fname>\n", argv[0]);
        return 1;
    }

    // Read the message from the file
    FILE *input_file = fopen(input_fname, "r");
    if (input_file == NULL)
    {
        perror("Failed to open input file");
        return 1;
    }
    
    // Get file size
    fseek(input_file, 0, SEEK_END);
    long file_size = ftell(input_file);
    fseek(input_file, 0, SEEK_SET);

    // Allocate buffer for message
    char *message = (char *)malloc(file_size + 1);
    if (message == NULL) {
        perror("Failed to allocate memory for message");
        fclose(input_file);
        return 1;
    }

    // Read file into buffer
    size_t bytes_read = fread(message, 1, file_size, input_file);
    message[bytes_read] = '\0'; // Null-terminate the string

    fclose(input_file);

    if ((long) bytes_read != file_size) {
        perror("Failed to read entire file");
        free(message);
        return 1;
    }

    char *server_log_fname = "benchmark_server.log";
    char *client_log_fname = "benchmark_client.log";
    protocol_benchmark(message, server_log_fname, client_log_fname);
    printf("Benchmark finished.\n");
    return 0;
}