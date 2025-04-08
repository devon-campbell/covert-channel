#ifndef RAS_TIME_H
#define RAS_H

#define RAS_SIZE 16

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

void update_stats(timing_stats* stats, uint64_t t);
void flush_ras(int count, int threshold);
void* flush_ras_threadable(void* count);
uint64_t recurse_and_yield(int depth, int count);
void* recurse_and_yield_threadable(void* arg);

#endif