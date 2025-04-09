#include "utils.h"

#define BYTES 64
#define SEND_PORT 3024
#define RECV_PORT 3025
#define SPIN_NUM 100000000
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

int create_and_bind(int port) {
    int server_fd;
    struct sockaddr_in address;
    
    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        return -1;
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("Setsockopt failed");
        close(server_fd);
        return -1;
    }
    
    // Setup address structure
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    
    // Bind to the port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        return -1;
    }
    
    // Start listening
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        close(server_fd);
        return -1;
    }
    
    return server_fd;
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

int send_data(const char *data, int n){
    int f_count = 0;
    
    // Start overall timing
    
    int sync_send = SEND_PORT;
    int sync_recv = RECV_PORT;
    int data_ports[BYTES*8];
    for (int i = 1; i <= BYTES*8; i++) {
        data_ports[i-1] = sync_recv + i;
    }
    
    // splitting the data into BYTES byte chunks
    int open[BYTES*8] = {0};
    for(int i = 0; i < n; i += BYTES) {
        // Start timing this chunk
        
        // Binding sync_send port
        int sync_fd = create_and_bind(sync_send);
        
        if (sync_fd < 0) {
            perror("Failed to create sync port");
            return -1;
        }
        
        // Closing previously open ports
        for(int j = 0; j < BYTES*8; j++) {
            if (open[j] != 0) {
                close(open[j]);
                open[j] = 0;
            }
        }
        
        for(int j = 0; j < BYTES && (i + j) < n; j++) {
            for(int k = 7; k >= 0; k--) {
                if ((data[i + j] & (1 << k)) == 0) {
                    int fd = create_and_bind(data_ports[j * 8 + k]);
                    if (fd < 0) {
                        perror("Failed to create data port");
                        close(sync_fd);
                        return -1;
                    }
                    open[j * 8 + k] = fd;
                }
                else {
                    open[j * 8 + k] = 0;
                }
            }
        }
        
        struct timespec start, current;
        clock_gettime(CLOCK_REALTIME, &start);
        // Releasing sync_send and waiting for sync_recv
        close(sync_fd);
        
        // Wait for sync_recv port to be occupied
        int flag = 0;
        while(check_port(sync_recv)) {
            clock_gettime(CLOCK_REALTIME, &current);
            if((current.tv_sec - start.tv_sec) + 
               (current.tv_nsec - start.tv_nsec) / 1e9 > 0.005){
                 
                fprintf(stderr, "Timeout waiting for sync_recv port\n");
                if (f_count > 1) {
                    return -1;
                }
                f_count++;
                flag = 1;
                break;
            }
        }
        if(flag == 0){
            f_count = 0;
        }
    }
    
    
    return 1;
}

Data * recv_data() {
    int sync_send = SEND_PORT;
    int sync_recv = RECV_PORT;
    int data_ports[BYTES*8];
    for (int i = 1; i <= BYTES * 8; i++) {
        data_ports[i-1] = sync_recv + i;
    }
    Data *data = make_data(100000);
    
    // Profiling variables
    
    // Start overall timing
    
    // Wait for sync port to be occupied
    while(check_port(sync_send)) ;
    
    while(1) {
        // Start timing this chunk
        
        // Check if resize is needed
        if (data->length+BYTES >= data->size) {
            data = double_data(data);
        }
        
        // Wait for sync_send to be available
        while(!check_port(sync_send)) ;
        
        // Create sync port
        int sync_fd = create_and_bind(sync_recv);
        
        if (sync_fd < 0) {
            perror("Failed to create sync port");
            return NULL;
        }
        
        // Process data
        for(int i = 0; i < BYTES; i++){
            char current = 0;
            for(int j = 7; j >= 0; j--){
                int port_status = check_port(data_ports[i * 8 + j]);
                current = current | (port_status << j);
            }
            data->data[data->length] = current;
            data->length++;
        }
        
        struct timespec start, current;
        
        // Close sync port and wait
        clock_gettime(CLOCK_REALTIME, &start);
        close(sync_fd);
        
        // Wait for sync_send port to be occupied again
        while(check_port(sync_send)) {
            clock_gettime(CLOCK_REALTIME, &current);
            
            if(current.tv_sec - start.tv_sec > 2) {
                fprintf(stderr, "Timeout waiting for sync_send port\n");
                return data;
            }
        }

        // Calculate total chunk time
    }
    
    return data;
}