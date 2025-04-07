#define _GNU_SOURCE
#define RAS_SIZE 16
#include "receive.h"
#include "print_utils.h"
#include <pthread.h>
#include <sched.h>

typedef struct {
    uint64_t count;
    double sum;
    long double sum_sq;
} timing_stats;

// For threading and threshold
typedef struct time_vals {
    int count;
    uint64_t start;
    uint64_t delta;
} time_vals;

static inline void flush_ras(int count, int threshold){
    if (count == threshold) {
        sched_yield();
        return;
    }
    else flush_ras(++count, threshold);
}

// Modified for threaded use in thresholding method
static inline void* flush_ras_threadable(void* count){
    if (*(int*) count == RAS_SIZE) {
        sched_yield();
        return NULL;
    } else {
        (*(int*) count)++;
        flush_ras_threadable(count);
        return NULL;
    }
}

// Fill RAS and measure return time after an interval
static inline uint64_t recurse_and_yield(int depth, int count){
    if (count == depth){
        // Yield to transmitter process
        sched_yield();
        return rdtscp64();
    }else{
        if (count == 0){  // Last to return (assuming depth > 0)
            uint64_t start = recurse_and_yield(depth, count+1);
            return rdtscp64() - start;
        }
        else return recurse_and_yield(depth, count+1);
    }
}

// Fill RAS and measure return time after an interval - threadable
static inline void* recurse_and_yield_threadable(void* arg){
    time_vals *tvals = (time_vals*) arg;
    int count = tvals->count;
    if (count == RAS_SIZE){
        // Yield to transmitter process
        sched_yield();
        tvals->start = rdtscp64();
        return NULL;
    }else{
        ++(tvals->count);
        if (count == 0){  // Last to return (assuming depth > 0)
            recurse_and_yield_threadable(tvals);
            tvals->delta = rdtscp64() - tvals->start;
            return NULL;
        }else{
            recurse_and_yield_threadable(tvals);
            return NULL;
        }
    }
}

// Measure and set detection threshold for a RAS flush 
static uint64_t tune_threshold(){
    uint64_t iterations = 0;
    uint64_t iterlim = 10000000;
    uint64_t det_threshold;
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
    return det_threshold;
}

static inline bool receive_bit(void *target_address){
    // Wait until time step A
    uint32_t initial = start_sync();

    uint64_t total_ret_time = 0;
    uint64_t total_returns = 0;
    uint32_t return_time;
    uint64_t det_threshold = tune_threshold();

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

// Receive a byte frame: state machine that waits for the preamble, start delimiter, reads data and parity
inline frame_t *receive_byte_frame(void *target_address, frame_t *frame_buf, uint64_t timeout)
{
    memset(frame_buf, 0, sizeof(frame_t));

    // In waiting state, look for the start delimiter with 8 bit shift register
    uint8_t shift_reg = 0;
    // start timeout time (microseconds)
    uint64_t start_time = rdtscp64();

    printf("[receive_byte_frame] Waiting until %lx cycles for start delimiter...\n", timeout);
    while (1) {
        // Shift in the next bit
        shift_reg = (shift_reg << 1) | receive_bit(target_address);
        
        // // Print shift register in binary
        // printf("Shift register: ");
        // for (int i = 7; i >= 0; i--) {
        //     printf("%d", (shift_reg >> i) & 1);
        // }
        // printf("\n");

        // fflush(stdout);
        // Check for start delimiter (0b10011010)
        if (shift_reg == START_DELIMITER) {
           // printf("New frame detected\n");
           // fflush(stdout);
            break; // Found start delimiter
        }
        // Check for timeout
       fflush(stdout);
        if (timeout > 0 && (rdtscp64() - start_time) > timeout) {
            printf("Timeout waiting for start delimiter\n");
            return NULL; // Timeout occurred
        }
    }

    // Save shift register as start delimiter
    byte_to_bools(shift_reg, frame_buf->start_delimiter);

    // In receiving state, read the next 8 bits for data and 1 bit for parity
    for (int i = 0; i < 8; i++) {
        frame_buf->data[i] = receive_bit(target_address);
    }
    frame_buf->parity = receive_bit(target_address);

    // Check frame validity
    if (!valid_frame(*frame_buf)) {
        printf("Invalid frame received\n");
        print_frame(*frame_buf);
        return NULL;
    }
   printf("[receive_byte_frame] Valid frame received\n");
    print_frame(*frame_buf);
    return frame_buf;
}