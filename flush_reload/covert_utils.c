

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
#define MAX_BUFFER_LEN 1024

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

inline __attribute__((always_inline)) void flush_event(uint64_t addr)
{
    asm volatile("clflush (%0)" ::"r"(addr));
}

static uint32_t era_start;
static uint32_t call_times[4096];
static uint32_t start_times[4096];
static uint32_t half_times[4096];

uint32_t measure_one_block_access_time(uint64_t addr)
{
    uint32_t cycles;

    asm volatile("mov %1, %%r8\n\t"
                 "lfence\n\t"
                 "rdtsc\n\t"
                 "mov %%eax, %%edi\n\t"
                 "mov (%%r8), %%r8\n\t"
                 "lfence\n\t"
                 "rdtsc\n\t"
                 "sub %%edi, %%eax\n\t"
                 : "=a"(cycles) /*output*/
                 : "r"(addr)
                 : "r8", "edi");

    return cycles;
}

// Measure access time for target address when flushed versus not flushed
static uint64_t miss_threashold = 100;

static void tune_threshold(void *target_address)
{
    uint64_t total_hit_time = 0;
    uint64_t total_miss_time = 0;
    uint64_t total_hits = 0;
    uint64_t total_misses = 0;
    uint64_t iters = 1000;

    while (total_hits < iters && total_misses < iters)
    {
        // Randomly choose to flush or not
        bool do_flush = rand() % 2;
        if (do_flush)
        {
            // Flush the cache line to force a miss
            flush_event((uint64_t)target_address);
        }
        else
        {
            // Access the cache line
            // int ret = access_event();
            uint32_t ret = measure_one_block_access_time((uint64_t)target_address);
        }

        // Measure time to access the cache line
        uint32_t t = measure_one_block_access_time((uint64_t)target_address);

        if (do_flush)
        {
            total_misses++;
            total_miss_time += t;
        }
        else
        {
            total_hits++;
            total_hit_time += t;
        }
    }

    // Calculate average hit and miss times
    uint64_t avg_hit_time = total_hit_time / total_hits;
    uint64_t avg_miss_time = total_miss_time / total_misses;
    printf("Average hit time: %lu\n", avg_hit_time);
    printf("Average miss time: %lu\n", avg_miss_time);

    // Set threshold to the average of hit and miss times
    miss_threashold = (avg_hit_time + avg_miss_time) / 2;
    printf("Threshold: %lu\n", miss_threashold);
    fflush(stdout);
}

// Send either high or low for a given number of cycles
static inline void send_bit(void *target_address, bool bit, int idx)
{

    // call_times[idx] = get_cycles();

    // Wait until time step A
    uint32_t initial = start_sync();
    start_times[idx] = initial;

    if (bit)
    {
        // Send until time step B
        while (!is_half_point())
        {
            flush_event((uint64_t)target_address);
        }
    }
    else
    {
        while (!is_half_point())
            ;
    }

    half_times[idx] = get_cycles();
}

static inline bool receive_bit(void *target_address, int idx)
{
    uint64_t total_access_time = 0;
    uint64_t total_accesses = 0;
    uint32_t access_time;
    // Wait until time step A
    uint32_t initial = start_sync();
    start_times[idx] = initial;

    // Check until time step B
    while (!is_half_point())
    {
        access_time = measure_one_block_access_time((uint64_t)target_address);
        total_access_time += access_time;
        total_accesses++;
    }

    // Calculate average access time
    uint64_t avg_access_time = total_access_time / total_accesses;
    

    // Compare with threshold
    bool out_bit = (avg_access_time > miss_threashold);

    return out_bit;
}
