/* ***********************************************
 * receiver.c
 * ----------------------------------------------
 * RAS covert channel receiver - calls nested  
 * function, yields control, and times return
 * to detect mispredictions caused by sender's RAS
 * flush.
 * *********************************************** */

#include <sched.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <x86intrin.h>

// Depth of test recursion
#define REC_DEPTH 16
// Will depend on depth of recursion, size of RAS, CPU, etc.
// Use threshold.c to fine tune
#define THRESHOLD 370

// Read timestamp counter
static inline uint64_t rdtscp64() {
    unsigned aux;
    return __rdtscp(&aux);
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

int main(){
    while(1){
        uint64_t delay = recurse_and_yield(REC_DEPTH, 0);
        int bit = (delay > THRESHOLD) ? 1 : 0;
        printf("%d", bit);
        // printf(" | Delay: %lu\n", delay);
        fflush(stdout);
    }

    return 0;
}