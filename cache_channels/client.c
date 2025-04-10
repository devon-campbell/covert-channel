#define _GNU_SOURCE
#include "arq_protocol.h"
#include "utils.h"

#include <stdio.h>
#include <sched.h>
#include <stdlib.h>

// Usage: ./client <bytestream> <logfile>
int main(int argc, char *argv[])
{

    char *log_fname = "server_recv.txt";
    char *message;

    if (argc == 2)
    {
        message = argv[1];
    }
    else if (argc == 3)
    {
        message = argv[1];
        log_fname = argv[2];
    }
    else
    {
        printf("Usage: %s <message> <opt:logfile>\n", argv[0]);
        return 1;
    }
    set_log_fd(stderr);
    cpu_set_t client_mask;
    CPU_ZERO(&client_mask);
    CPU_SET(0, &client_mask);
    if (sched_setaffinity(0, sizeof(cpu_set_t), &client_mask) < 0)
    {
        perror("Failed to set CPU affinity for server");
        exit(1);
    }

    arq_client(message, log_fname);
    printf("Client finished.\n");
    return 0;
}