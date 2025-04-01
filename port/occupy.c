#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <time.h>

volatile sig_atomic_t running = 1;

void handle_signal(int sig) {
    printf("\nClosing server...\n");
    running = 0;
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

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    
    // Register signal handler for clean shutdown with Ctrl+C
    signal(SIGINT, handle_signal);
    
    printf("Starting port cycling on port %d (10 times/second)...\n", port);
    printf("Press Ctrl+C to exit.\n");
    
    int cycle_count = 0;
    
    // Cycle between binding and releasing the port
    while(running) {
        // Bind to port
        int server_fd = create_and_bind(port);
        if (server_fd < 0) {
            usleep(100000); // Wait 100ms before trying again
            continue;
        }
        
        // printf("\rCycle: %d - Port %d bound   ", ++cycle_count, port);
        fflush(stdout);
        
        // Hold the port for 10ms
        usleep(10000);
        
        // Release port
        close(server_fd);
        
        // printf("\rCycle: %d - Port %d released", cycle_count, port);
        fflush(stdout);
        
        // Wait 10ms before next cycle
        usleep(10000);
    }
    
    printf("\nCompleted %d cycles\n", cycle_count);
    return 0;
}