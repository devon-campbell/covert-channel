#include "send.h"
#include "receive.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>
#include <sched.h>
#include <stdio.h>

/* HELPERS */

// To tune this, start FR_PERIOD_SHIFT a high value (e.g. 24) and decrease until accuracy breaks down or
// throughput stops improving
// Note that if you start to get FP/Integer division errors, the period is likely too short and the receiver is not getting
// any samples.
#define FR_PERIOD_SHIFT 17

// The sync range only needs to be large enough to account for random fluctuation in calls, and small enough relative to the
// period
#define FR_SYNC_RANGE 0x50





#define FR_PERIOD_CYCLES ((1 << FR_PERIOD_SHIFT) - 1)  // Mask FR_PERIOD_SHIFT bits
// #define FR_PERIOD_CYCLES 0x000FFFFF
#define CHANNEL_HALF_MAX (FR_PERIOD_CYCLES >> 1)

// #define FR_PERIOD_CYCLES 0xFFFFFF
// #define CHANNEL_HALF_MAX (FR_PERIOD_CYCLES >> 1)
// #define FR_SYNC_RANGE 0x100



// Get the current time in cycles
static FORCE_INLINE uint32_t get_cycles(void)
{
    uint32_t cycles;
    asm volatile("rdtscp" : "=a"(cycles));

    return cycles;
}

static FORCE_INLINE uint32_t cycle_in_period() {
    return get_cycles() & FR_PERIOD_CYCLES;
}


static FORCE_INLINE uint32_t start_sync()
{
    // Spin until time stamp counter overflows
    while ((cycle_in_period()) > FR_SYNC_RANGE)
    {
    }
    return get_cycles();
}

static FORCE_INLINE bool is_half_point()
{
    // If counter greater than half max, we are in the second half of the cycle
    return (cycle_in_period()) > CHANNEL_HALF_MAX;
}

static FORCE_INLINE void flush_event(uint64_t addr)
{
    asm volatile("clflush (%0)" ::"r"(addr));
}

static FORCE_INLINE uint32_t time_access(uint64_t addr)
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
                 : "=a"(cycles)
                 : "r"(addr)
                 : "r8", "edi");

    return cycles;
}

uint64_t measure_thresh(void *target_address)
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
            uint32_t ret = time_access((uint64_t)target_address);
        }

        // Measure time to access the cache line
        uint32_t t = time_access((uint64_t)target_address);

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
    info("Average hit time: %lu\n", avg_hit_time);
    info("Average miss time: %lu\n", avg_miss_time);

    uint64_t miss_threashold = avg_hit_time + ((avg_miss_time - avg_hit_time) / 4);
    info("Threshold: %lu\n", miss_threashold);
    fflush(stdout);

    return miss_threashold;
}

static void *thresh_addr = NULL;
static FORCE_INLINE void *get_thresh_addr(void)
{
    if (thresh_addr != NULL)
    {
        return thresh_addr; // Return cached address if already set
    }

    thresh_addr = shared_file_address("/bin/less", 0);
    return thresh_addr;
}

static uint64_t miss_threshold = 0;
static FORCE_INLINE uint64_t check_thresh(void)
{

    void *thresh_addr = get_thresh_addr();
    if (thresh_addr == NULL)
    {
        // error("Error: Could not get threshold address.\n");
        return 0;
    }

    if (miss_threshold == 0)
    {
        miss_threshold = measure_thresh(thresh_addr);
    }
    // TODO re-tune

    return miss_threshold;
}

/* MAIN FUNCTIONS */

void calibrate_bit_channel()
{
    // Get the threshold for the bit channel
    check_thresh();
}

// Send either high or low for a given number of cycles
FORCE_INLINE void send_bit(void *target_address, bool bit)
{

    // Wait until time step A
    uint32_t initial = start_sync();

    if (bit)
    {
        // Send until time step B
        while (!is_half_point())
        {
            flush_event((uint64_t)target_address);
            // yield
            sched_yield();
        }
    }
    else
    {
        while (!is_half_point())
            ;
    }

    //   print_bit(bit, "Sent: ");
}

FORCE_INLINE bool receive_bit(void *target_address)
{
    uint64_t total_access_time = 0;
    uint64_t total_accesses = 0;
    uint32_t access_time;

    uint64_t threshold = check_thresh();
    // Wait until time step A
    uint32_t initial = start_sync();

    // Check until time step B
    while (!is_half_point())
    {
        access_time = time_access((uint64_t)target_address);
        total_access_time += access_time;
        total_accesses++;
        sched_yield();
    }

    // Calculate average access time
    uint64_t avg_access_time = total_access_time / total_accesses;

    // print the average access time
    //    printf("Avg access time: %lu\t\t", avg_access_time);
    //    printf("Threshold: %lu\t\t", threshold);

    // Compare with threshold
    bool out_bit = (avg_access_time > threshold);

    // print the bit
    // printf("Received bit: %d\n", out_bit);
    // fflush(stdout);
    //  print_bit(out_bit, "Received: ");
    return out_bit;
}
