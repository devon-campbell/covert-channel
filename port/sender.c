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

// // This is a program to print all avaliable ports
// int create_and_bind(int port) {
//     int server_fd;
//     struct sockaddr_in address;
    
//     // Create socket
//     if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
//         perror("Socket creation failed");
//         return -1;
//     }
    
//     // Set socket options
//     int opt = 1;
//     if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
//         perror("Setsockopt failed");
//         close(server_fd);
//         return -1;
//     }
    
//     // Setup address structure
//     address.sin_family = AF_INET;
//     address.sin_addr.s_addr = INADDR_ANY;
//     address.sin_port = htons(port);
    
//     // Bind to the port
//     if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
//         perror("Bind failed");
//         close(server_fd);
//         return -1;
//     }
    
//     // Start listening
//     if (listen(server_fd, 3) < 0) {
//         perror("Listen failed");
//         close(server_fd);
//         return -1;
//     }
    
//     return server_fd;
// }

// void wait_for_time_boundary(int bits) {
//     struct timespec start, ts;
//     clock_gettime(CLOCK_REALTIME, &start);
//     start.tv_nsec += (1 << bits);
//     //masking out the lower bits to 0
//     start.tv_nsec &= ~((1 << bits) - 1);
//     int seconds = 0;
//     if (start.tv_nsec >= 1000000000) {
//         seconds = start.tv_nsec / 1000000000;
//         start.tv_nsec -= seconds * 1000000000;
//     }

    

//     while (1) {
//         clock_gettime(CLOCK_REALTIME, &ts);
//         if(ts.tv_nsec >= start.tv_nsec && ts.tv_sec >= start.tv_sec + seconds) {
//             break;
//         }
//     }
// }

// int send_data_old(char *data, int n){
//     int sync_port = 2024;
//     int data_ports[BYTES*8];
//     for (int i = 1; i <= BYTES*8; i++) {
//         data_ports[i-1] = sync_port + i;
//     }
//     // occupying the sync port
//     int sync_fd = create_and_bind(sync_port);
//     if (sync_fd < 0) {
//         perror("Failed to create sync port");
//         return -1;
//     }
//     // splitting the data into 4 byte chunks
//     int open[BYTES*8] = {0};
//     wait_for_time_boundary(31);
//     for(int i = 0; i < n; i += BYTES) {
//         wait_for_time_boundary(21);
//         //closing all of them
//         for(int j = 0; j < BYTES*8; j++) {
//             if (open[j] != 0) {
//                 // closing the port
//                 close(open[j]);
//                 open[j] = 0;
//             }
//         }
        
//         // looking at each bit, and binding the port if it is high
//         struct timespec step_start, step_end;
//         clock_gettime(CLOCK_MONOTONIC, &step_start);

//         for(int j = 0; j < BYTES; j++) {
//             for(int k = 7; k >= 0; k--) {
//             // printf("%d\n", data_ports[j * 8 + k]);
//             if ((data[i + j] & (1 << k)) == 0) {
//                 int fd = create_and_bind(data_ports[j * 8 + k]);
//                 if (fd < 0) {
//                 perror("Failed to create data port");
//                 close(sync_fd);
//                 return -1;
//                 }
//                 open[j * 8 + k] = fd;
//             }
//             else {
//                 open[j * 8 + k] = 0;
//             }
//             }
//             for (int k = 0; k < 8; k++) {
//             // printf("%d", (data[i + j] >> k) & 1);
//             }
//             // printf(" %c ", data[i + j]);
//         }

//         clock_gettime(CLOCK_MONOTONIC, &step_end);
//         double step_time = (step_end.tv_sec - step_start.tv_sec) + 
//                    (step_end.tv_nsec - step_start.tv_nsec) / 1e9;
//         // printf("\nStep time: %.6f seconds\n", step_time);
//         // printf("\n");
//         // printing the current time
//         struct timespec ts;
//         clock_gettime(CLOCK_REALTIME, &ts);
//         // printf("Time: %ld.%09ld\n", ts.tv_sec, ts.tv_nsec);
//         // waiting for the time boundary
        
//     }
//     close(sync_fd);
//     wait_for_time_boundary(29);
//     for(int j = 0; j < 32; j++) {
//         if (open[j] != 0) {
//             // closing the port
//             close(open[j]);
//             open[j] = 0;
//         }
//     }
//     return 1;
// }

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