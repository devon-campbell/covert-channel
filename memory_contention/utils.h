#ifndef MEMORY_CONTENTION_UTILS_H
#define MEMORY_CONTENTION_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <stdint.h>

// Constants for DRAM covert channel
#define DRAM_THRESHOLD_NS 200     // Threshold for distinguishing between fast/slow memory access
#define SHARED_MEM_SIZE 4096      // Size of shared memory region
#define SHARED_MEM_NAME "/dram_covert_channel"  // Name of shared memory region

// Structure for storing received data
typedef struct {
    char *data;
    int length;
    int size;
} Data;

// Memory bus saturation functions
void saturate_memory_bus(int duration_us);
uint64_t measure_dram_access_time();

// Data transmission functions
int send_data(const char *data, int n);
Data *recv_data();

// Data structure helper functions
Data *make_data(int size);
Data *double_data(Data *data);



#endif // MEMORY_CONTENTION_UTILS_H