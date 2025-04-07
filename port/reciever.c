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

#define BYTES 16

typedef struct {
    int length;
    int size;
    char *data;
} Data;

// instianting the array
Data * make_data(int n) {
    Data *data = (Data *) malloc(sizeof(Data));
    data->length = 0;
    data->size = n;
    data->data = (char *)malloc(n);
    return data;
}

//dynamically allocating the data
Data * double_data(Data *data) {
    data->size *= 2;
    char * newy = (char *)malloc(data->size);
    memcpy(newy, data->data, data->length);
    free(data->data);
    data->data = newy;
    return data;
}


void wait_for_time_boundary_half(int bits) {
    struct timespec start, ts;
    clock_gettime(CLOCK_REALTIME, &start);
    // printf("Start time: %1ld.%09ld\n", start.tv_sec, start.tv_nsec);
    start.tv_nsec += (1 << bits);
    //masking out the lower bits to 0
    start.tv_nsec &= ~((1 << bits) - 1);
    start.tv_nsec |= (1 << (bits-1));
    int seconds = 0;
    if (start.tv_nsec >= 1000000000) {
        seconds = start.tv_nsec / 1000000000;
        start.tv_nsec -= seconds * 1000000000;
    }
    // printf("Start time: %1ld.%09ld\n", start.tv_sec, start.tv_nsec);

    while (1) {
        clock_gettime(CLOCK_REALTIME, &ts);
        // printf("Start time: %1ld.%09ld\n", ts.tv_sec, ts.tv_nsec);
        if(ts.tv_nsec >= start.tv_nsec && ts.tv_sec >= start.tv_sec + seconds) {
            break;
        }
    }
}

void wait_for_time_boundary(int bits) {
    struct timespec start, ts;
    clock_gettime(CLOCK_REALTIME, &start);
    start.tv_nsec += (1 << bits);
    //masking out the lower bits to 0
    start.tv_nsec &= ~((1 << bits) - 1);
    int seconds = 0;
    if (start.tv_nsec >= 1000000000) {
        seconds = start.tv_nsec / 1000000000;
        start.tv_nsec -= seconds * 1000000000;
    }

    

    while (1) {
        clock_gettime(CLOCK_REALTIME, &ts);
        if(ts.tv_nsec >= start.tv_nsec && ts.tv_sec >= start.tv_sec + seconds) {
            break;
        }
    }
}

int check_port(int port) {
    int sock;
    struct sockaddr_in addr;
    
    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket failed");
        return 0;
    }
    
    // Enable quick reuse of the address
    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(sock);
        return 0;
    }
    
    // Set up address structure
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    // Try to bind to the port
    int result = bind(sock, (struct sockaddr*)&addr, sizeof(addr));
    close(sock);
    
    if (result < 0) {
        return 0;  // Port is not available
    }
    return 1;      // Port is available
}

Data * recv_data() {
    int sync_port = 2024;
    int data_ports[BYTES*8];
    for (int i = 1; i <= BYTES * 8; i++) {
        data_ports[i-1] = sync_port + i;
    }
    Data *data = make_data(100000);
    //spinning until sync_port is occupied
    while(1) {
        if(!check_port(sync_port)) {
            break;
        }
    }
    // printf("Sync port occupied\n");
    wait_for_time_boundary(31);
    while(1) {
        while (data->length+4 >= data->size) {
            data = double_data(data);
        }
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);

        wait_for_time_boundary_half(21);

        clock_gettime(CLOCK_MONOTONIC, &end);
        double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
        // printf("Waited for: %.9f seconds\n", elapsed_time);
        for(int i = 0; i < BYTES; i++){
            
            char current = 0;
            for(int j = 7; j >= 0; j--){
                // printf("%d\n", data_ports[i * 8 + j]);
                current = current | ((check_port(data_ports[i * 8 + j]) << j));
            }
            data->data[data->length] = current;
            for (int k = 0; k < 8; k++) {
            // printf("%d", (current >> k) & 1);
            }
            // printf(" %c ", current);
            data->length++;
        }

        // clock_gettime(CLOCK_MONOTONIC, &end);
        // double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
        // printf("Step execution time: %.9f seconds\n", elapsed_time);
        // printf("\n");
        if(check_port(sync_port)){
            break;
        }
        // struct timespec ts;
        // clock_gettime(CLOCK_REALTIME, &ts);
        // printf("Time: %1ld.%09ld\n", ts.tv_sec, ts.tv_nsec);
    }
    return data;
}

int main(){
    Data *data = recv_data();
    // printf("Received data length: %d\n", data->length);
    for(int i = 0; i < data->length; i++){
        printf("%c", data->data[i]);
    }
    printf("\n");
    free(data->data);
    free(data);
    return 0;
}