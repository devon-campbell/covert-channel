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


// Program to find available ports for a web server

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

int main(int argc, char *argv[]) {
    int start_port = 1024;  // Ports below 1024 require root privileges
    int end_port = 10000;   // Limit to a reasonable range
    
    // Parse command line arguments if provided
    if (argc >= 2) {
        start_port = atoi(argv[1]);
    }
    if (argc >= 3) {
        end_port = atoi(argv[2]);
    }
    
    printf("Scanning for available ports from %d to %d...\n", start_port, end_port);
    printf("Unavailable ports:\n");
    
    int available_count = 0;
    int switches = 0;
    time_t start_time = time(NULL);
    int old = 0;
    while(1) {
        int port = 1234;
        if (!check_port(port)) {
            if (old) {
                switches++;
            }
            old = 0;
        } else {
            if (!old) {
                switches++;
            }
            old = 1;
        }

        // Print switches per second every second
        if (time(NULL) - start_time >= 1) {
            printf("Switches per second: %d\n", switches);
            switches = 0;
            start_time = time(NULL);
        }
    }
    
    printf("\n\nTotal available ports: %d\n", available_count);
    return 0;
}