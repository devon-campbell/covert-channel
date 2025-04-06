/* ***********************************************
 * threshold.c
 * ----------------------------------------------
 * Once RAS size is known, measure difference in
 * time with and without flushing.
 * *********************************************** */

#define _GNU_SOURCE
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <x86intrin.h>
#include <math.h>
#include <time.h>

#define REPORT_INTERVAL 100000 // how often to report stats
#define REC_DEPTH 16  // depth of recursion (set to size of RAS)

// Read timestamp counter
static inline uint64_t rdtscp64() {
    unsigned aux;
    return __rdtscp(&aux);
}

typedef struct {
    uint64_t count;
    double sum;
    long double sum_sq;
} timing_stats;

void update_stats(timing_stats* stats, uint64_t t) {
    stats->count++;
    stats->sum += t;
    stats->sum_sq += (long double)t * t;
}

void print_stats(const char* label, timing_stats* stats) {
    if (stats->count == 0) return;
    double avg = stats->sum / stats->count;
    double variance = (stats->sum_sq - (long double)(stats->sum * avg)) / stats->count;
    double stddev = sqrt(variance);
    fprintf(stderr, "%s: count =%lu avg =%.2f cycles stddev =%.2f cycles\n", 
            label, stats->count, avg, stddev);
    fflush(stderr);
}

static inline void* flush_ras(void* count){
    if (*(int*) count == REC_DEPTH) {
        sched_yield();
        return NULL;
    } else {
        (*(int*) count)++;
        flush_ras(count);
        return NULL;
    }
}

typedef struct time_vals {
    int count;
    uint64_t start;
    uint64_t delta;
} time_vals;

// Fill RAS and measure return time after an interval
static inline void* recurse_and_yield(void* arg){
    time_vals *tvals = (time_vals*) arg;
    int count = tvals->count;
    if (count == REC_DEPTH){
        // Yield to transmitter process
        sched_yield();
        tvals->start = rdtscp64();
        return NULL;
    }else{
        ++(tvals->count);
        if (count == 0){  // Last to return (assuming depth > 0)
            recurse_and_yield(tvals);
            tvals->delta = rdtscp64() - tvals->start;
            return NULL;
        }else{
            recurse_and_yield(tvals);
            return NULL;
        }
    }
}

int main(){
    uint64_t iterations = 0;
    timing_stats flushed = {0}, nonflushed = {0};
    srand(time(NULL));

    while (1) {
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
            
            pthread_create(&ptime, NULL, recurse_and_yield, (void*) &tv);
            pthread_create(&pflush, NULL, flush_ras, (void*) &cnt);

            pthread_setaffinity_np(ptime, sizeof(cpu_set_t), &cpuset);
            pthread_setaffinity_np(pflush, sizeof(cpu_set_t), &cpuset);

            pthread_join(pflush, NULL);
            pthread_join(ptime, NULL);
            t = tv.delta;
        }else{
            // Don't spawn pthread for flushing RAS
            recurse_and_yield(&tv);
            t = tv.delta;
        }

        // if (t >= 1500) printf("Outlier: %ld\n", t);
        if (do_flush)
            update_stats(&flushed, t);
        else
            update_stats(&nonflushed, t);

        iterations++;

        if (iterations % REPORT_INTERVAL == 0) {
            fprintf(stderr, "\n- - - Stats after %lu iterations - - -\n", iterations);
            print_stats("Flushed", &flushed);
            print_stats("Non-Flushed", &nonflushed);
        }
    }
    return 0;
}