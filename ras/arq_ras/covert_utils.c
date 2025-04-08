
#define _GNU_SOURCE
#include <pthread.h>
#include <stdint.h>
#include <x86intrin.h> // for _mm_clflush, __rdtscp
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>
#include <ctype.h>
#include <fcntl.h>     // for open
#include <sys/mman.h>  // for mmap
#include <sys/types.h> // for size_t
#include <sys/stat.h>  // for open
#include <errno.h>     // for errno
#include <dlfcn.h>

#include "ras_time.h"

// In testing, time to loop is usually less than 250 cycles
// Time to sync is usually around  ~1M cycles
#define CHANNEL_SYNC_MAX 0x000FFFFF
#define CHANNEL_HALF_MAX (CHANNEL_SYNC_MAX >> 1)
#define CHANNEL_SYNC_JITTER 0x1000

// #define BIT_CYCLE_COUNT 7587204

// Overall timeline (starting at a sync point)
// 0 to 1000 cycles: both sender and receiver unblock and start to run (TIME STEP A)
// 1000 to 500k cycles: sender flushes target address, receiver checks target address
// 500k to 1M cycles: sender and receiver wait for next sync point (TIME STEP B)

#define DEFAULT_FILE_NAME "/bin/ls"
#define DEFAULT_FILE_OFFSET 0x0
#define DEFAULT_FILE_SIZE 4096
#define CACHE_BLOCK_SIZE 64
#define RAS_SIZE 16
#define MAX_BUFFER_LEN 1024

static uint32_t start_times[4096];
static uint32_t half_times[4096];

static void *event_address = NULL;
static inline void *get_event_address()
{
    if (event_address != NULL)
    {
        return event_address; // Return cached address if already set
    }

    char *filename = DEFAULT_FILE_NAME;

    int inFile = open(filename, O_RDONLY);
    if (inFile == -1)
    {
        printf("Failed to Open File\n");
        exit(1);
    }

    void *mapaddr = mmap(NULL, DEFAULT_FILE_SIZE, PROT_READ, MAP_SHARED, inFile, 0);

    if (mapaddr == (void *)-1)
    {
        printf("Failed to Map Address\n");
        exit(1);
    }

    event_address = (uint64_t)mapaddr + DEFAULT_FILE_OFFSET;

    // event_address = libc_fn; // Cache the address for future calls
    return event_address;
}

// Get the current time in cycles
static inline __attribute__((always_inline)) uint32_t get_cycles(void)
{
    uint32_t cycles;
    asm volatile("rdtscp" : /* outputs */ "=a"(cycles));

    return cycles;
}

static inline __attribute__((always_inline))
uint32_t
start_sync()
{
    // Spin until time stamp counter overflows
    while ((get_cycles() & CHANNEL_SYNC_MAX) > CHANNEL_SYNC_JITTER)
    {
    }
    return get_cycles();
}

static inline bool __attribute__((always_inline)) is_half_point()
{
    // If counter greater than half max, we are in the second half of the cycle
    return (get_cycles() & CHANNEL_SYNC_MAX) > CHANNEL_HALF_MAX;
}

// Measure and set detection threshold for a RAS flush 
static uint64_t det_threshold = 100;
static void tune_threshold(){
    uint64_t iterations = 0;
    uint64_t iterlim = 10000000;
    timing_stats flushed = {0}, nonflushed = {0};
    srand(time(NULL));

    while (iterations < iterlim) {
        int do_flush = rand() % 2;

        uint64_t t;
        time_vals tv = {0};
        if (do_flush){
            // Spawn pthreads for flushing RAS and timing execution
            pthread_t pflush, ptime;
            int cnt = 0;

            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);
            CPU_SET(0, &cpuset);  // Assign to core 0
            
            pthread_create(&ptime, NULL, recurse_and_yield_threadable, (void*) &tv);
            pthread_create(&pflush, NULL, flush_ras_threadable, (void*) &cnt);

            pthread_setaffinity_np(ptime, sizeof(cpu_set_t), &cpuset);
            pthread_setaffinity_np(pflush, sizeof(cpu_set_t), &cpuset);

            pthread_join(pflush, NULL);
            pthread_join(ptime, NULL);
            t = tv.delta;
        }else{
            // Don't spawn pthread for flushing RAS
            recurse_and_yield_threadable(&tv);
            t = tv.delta;
        }

        if (do_flush) update_stats(&flushed, t);
        else update_stats(&nonflushed, t);

        iterations++;
    }

    // Set threshold to average of flush and non-flush averages
    det_threshold = (flushed.sum/flushed.count + nonflushed.sum/nonflushed.count) / 2;
    printf("Threshold: %lu\n", det_threshold);
    fflush(stdout);
}

// Send either high or low for a given number of cycles
static inline void send_bit(void *target_address, bool bit, int idx){
    // Wait until time step A
    uint32_t initial = start_sync();
    start_times[idx] = initial;

    if (bit){
        // Send until time step B
        while (!is_half_point()){
            flush_ras(RAS_SIZE, det_threshold);
        }
    }
    else
    {
        while (!is_half_point())
            ;
    }

    half_times[idx] = get_cycles();
}

static inline bool receive_bit(void *target_address, int idx){
    // Wait until time step A
    uint32_t initial = start_sync();
    start_times[idx] = initial;

    uint64_t total_ret_time = 0;
    uint64_t total_returns = 0;
    uint32_t return_time;

    // Check until time step B
    while (!is_half_point())
    {
        return_time = recurse_and_yield(RAS_SIZE, 0);
        total_ret_time += return_time;
        total_returns++;
    }

    // Calculate average access time
    uint64_t avg_ret_time = total_ret_time / total_returns;
    
    // Compare with threshold
    bool out_bit = (avg_ret_time > det_threshold);

    return out_bit;
}
