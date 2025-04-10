#define _GNU_SOURCE
#include "arq_protocol.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <sched.h>

// Usage: ./server <logfile>
int main(int argc, char *argv[])
{

    char *log_fname;
    if (argc == 1)
    {
        // Default log file name
        log_fname = "server_recv.txt";
    }
    else if (argc == 2)
    {
        log_fname = argv[1];
    }
    else
    {
        printf("Usage: %s <logfile>\n", argv[0]);
        return 1;
    }

    // set_log_fd(stderr);
    // Pin to CPU 1
    cpu_set_t server_mask;
    CPU_ZERO(&server_mask);
    CPU_SET(1, &server_mask);
    if (sched_setaffinity(0, sizeof(cpu_set_t), &server_mask) < 0)
    {
        perror("Failed to set CPU affinity for server");
        exit(1);
    }

    printf("[server] Starting ARQ server...\n");
    arq_server(log_fname, 100000);
    printf("Server finished.\n");
    return 0;
}