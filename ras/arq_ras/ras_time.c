// Source file for RAS thresholding functions

#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <x86intrin.h>
#include <math.h>
#include <time.h>

#include "ras_time.h"
#include "covert_utils.h"  // for rdtscp

void update_stats(timing_stats* stats, uint64_t t) {
    stats->count++;
    stats->sum += t;
    stats->sum_sq += (long double)t * t;
}

void flush_ras(int count, int threshold){
    if (count == threshold) {
        sched_yield();
        return;
    }
    else flush_ras(++count, threshold);
}

// Modified for threaded use in thresholding method
void* flush_ras_threadable(void* count){
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
uint64_t recurse_and_yield(int depth, int count){
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
void* recurse_and_yield_threadable(void* arg){
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