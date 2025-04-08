#ifndef UTILS_H
#define UTILS_H

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
#include <stdint.h>
#include <sched.h>     // For struct sched_param, SCHED_FIFO, sched_setscheduler
#include <sys/mman.h>

typedef struct {
    int length;
    int size;
    char *data;
} Data;

Data * make_data(int n); 

//dynamically allocating the data
Data * double_data(Data *data);



// Function to initialize resources
void initialize_resources();

// Function to clean up resources
void cleanup_resources();

// Function to send data through the covert channel
int send_data(const char *data, int n);

// Function to receive data through the covert channel
Data * recv_data();

#endif // UTILS_H