/* ***********************************************
 * threshold.c
 * ----------------------------------------------
 * Once RAS size is known, measure difference in
 * time with and without flushing.
 * *********************************************** */

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

static inline void recurse(int depth, int count){
    if (count == depth) return;
    else recurse(depth, ++count);
}

// Returns time to return from nested calls
static inline uint64_t timed_recurse(int depth, int count, bool flush){
    if (count == depth){
        if (flush) recurse(depth, 0);  // Flush RAS of original return addresses
        return rdtscp64();  // Timestamp at start of unraveling
    }else{
        if (count == 0){  // Last to return (assuming depth > 0)
            uint64_t start = timed_recurse(depth, ++count, flush);
            return rdtscp64() - start;
        }
        else return timed_recurse(depth, ++count, flush);
    }
}

int main(){
    uint64_t iterations = 0;
    timing_stats flushed = {0}, nonflushed = {0};
    srand(time(NULL));

    while (1) {
        int do_flush = rand() % 2;

        uint64_t t = timed_recurse(REC_DEPTH, 0, do_flush);
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